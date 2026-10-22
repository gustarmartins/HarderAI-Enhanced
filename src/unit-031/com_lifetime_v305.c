/* Passive scene lifetime receipt for standalone COM tuning. No game writes.
 * Called after component-4 publication and BEFORE scene virtual cleanup.
 */
typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
#define MAGIC 0x31464c4d4f434148ull
extern void lab_golden_boundary_retire(void);
typedef struct {
    u64 magic;u32 abi,size;
    u64 sequence,generation,scene,system,race,group,scene_generation;
    u32 lock,active,fault,reserved;
} ComLifetime;
_Static_assert(sizeof(ComLifetime)==88,"COM lifetime ABI");
EXPORT ComLifetime lab_com_lifetime_record={.magic=MAGIC,.abi=2,.size=88};
static u64 q(const void *p,u32 off){return *(const u64 *)((const char *)p+off);}
static u32 w(const void *p,u32 off){return *(const u32 *)((const char *)p+off);}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x2000;}

EXPORT void lab_com_lifetime_update(ComLifetime *s,u64 base,u32 event,void *object,u64 selector){
    if(!s||s->magic!=MAGIC||s->abi!=2||s->size!=88||s->reserved||s->fault)return;
    if(!base||(base&4095)||base>=(1ull<<39)-0x1400000||!ptr((u64)object))return;
    u64 scene=0,system=0;
    if(event==1){
        /* Native 7F095C published this component at scene+1B0. */
        if(q(object,0)!=base+0x125a060||w(object,0x1d0)!=4)return;
        u64 parent=q(object,0x20);if(!ptr(parent))return;
        scene=q((void *)parent,0x20);if(!ptr(scene))return;
        if(q((void *)scene,0)!=base+0x125e158||selector!=scene+0x20||
           q((void *)scene,0x1b0)!=(u64)object)return;
        system=(u64)object;
    }else if(event==2){
        /* No dereference of a scene being retired is needed. */
        scene=(u64)object;if(scene!=s->scene)return;
    }else if(event==3||event==4){
        if(!ptr(s->system)||!ptr(s->scene)||q(object,0)!=base+0x11b4308||
           q((void *)s->system,0x238)!=(u64)object)return;
        if(event==4 && (!ptr(selector)||s->race!=(u64)object||
           q(object,0xb0)!=selector||q((void *)selector,0x28)!=(u64)object))return;
    }else if(event==5){
        /* Destructor entry, before any native cleanup/free. */
        if((u64)object!=s->race)return;
    }else return;
    u32 expected=0;
    if(!__atomic_compare_exchange_n(&s->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        /* Never silently miss a lifetime boundary under contention. */
        __atomic_store_n(&s->fault,1,__ATOMIC_RELEASE);return;
    }
    if(s->fault)goto done;
    if(s->sequence>~0ull-2 || ((event==1||event==4)&&s->generation==~0ull)){
        s->active=0;__atomic_store_n(&s->fault,2,__ATOMIC_RELEASE);goto done;
    }
    lab_golden_boundary_retire();
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_ACQ_REL);
    if(event==1){
        s->generation++;s->scene_generation=s->generation;s->scene=scene;s->system=system;
        s->active=0;s->race=0;s->group=0;
    }else if(event==2 && s->scene==scene){s->active=0;s->scene=0;s->system=0;s->race=0;s->group=0;}
    else if(event==3){s->active=0;s->race=(u64)object;s->group=0;}
    else if(event==4){s->generation++;s->group=selector;s->active=1;}
    else if(event==5 && s->race==(u64)object){s->active=0;s->race=0;s->group=0;}
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_RELEASE);
done:
    __atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);
}
