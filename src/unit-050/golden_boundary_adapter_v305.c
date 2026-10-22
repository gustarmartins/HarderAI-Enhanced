/* Source-only scalar transport and main-owned boundary-rank reducer.
 * Native allocation authority is the verified method-tree invocation, not
 * pointer bounds, the generation receipt or a second matching snapshot.
 * No native game-object writes; displaced stores are replayed in assembly.
 */
#include "golden_boundary_adapter_v305.h"
#include "queue_thread.h"
#include "item_task_collect.h"
extern char lab_module_start[];
#define EXPORT __attribute__((visibility("default")))
#define PHASE(v) ((u64)(v)<<32)
enum { G_WORKERS=1,G_COMMANDS=2,G_CLOSED=3,G_USE=4,G_TICK=6,G_EXPIRY=7,G_LIMIT=8 };
typedef union {u32 word;float value;} GFloat;
typedef struct {u64 inventory,driver,role,held;u32 golden,rank;float progress,scale;} GOwner;
typedef struct {
    u64 sequence,held;u32 kind,uses,previous,elapsed,duration,timed,limited,witness;
} GScalar;
typedef struct {u32 lock,count,fault,pad;u64 serial;GScalar records[GOLDEN_CAPTURE_CAPACITY];} GChannel;
typedef struct {
    u64 first_sequence,last_sequence,held;u32 active,start_tick,last_tick,start_rank,uses,elapsed;
    float start_progress,scale;
} GEpisode;
typedef struct {u32 full,pad;GoldenBoundaryEvidence value;} GMailbox;
typedef struct {
    u64 base,scene,system,race,group,manager,clock,epoch,generation,token,thread;
    /* roster_mask keeps current native CPUs through transient hit/recovery
     * states. mask is narrower: Golden event qualification for this update. */
    u32 tick,count,mode,mask,roster_mask;GOwner owners[12];
} GSnapshot;
typedef struct {
    u64 gate,serial;u32 fault,command_tail;GSnapshot scope;
    GChannel channel[12];GEpisode episode[12];GMailbox mailbox[12];
} GState;
EXPORT GState lab_golden_boundary_state;
EXPORT const u32 lab_golden_boundary_config[8]={1,sizeof(GoldenBoundaryEvidence),12,
    GOLDEN_CAPTURE_CAPACITY,GOLDEN_ITEM_EXIT_RANK_V1,0x3678,450,120};
static u64 gq(u64 p,u32 off){return *(const volatile u64 *)(p+off);}
static u32 gw(u64 p,u32 off){return *(const volatile u32 *)(p+off);}
static u32 gb(u64 p,u32 off){return *(const volatile u8 *)(p+off);}
static float gf(u64 p,u32 off){GFloat f={.word=gw(p,off)};return f.value;}
static u32 ptr(u64 p){return p>=0x10000 && !(p&7) && p<(1ull<<39)-0x3000;}
static u32 finite(float f){GFloat v={.value=f};return (v.word&0x7fffffffu)<0x7f800000u;}
static u64 thread(void){u64 t;__asm__ volatile("mrs %0,tpidrro_el0":"=r"(t));return t;}
static u64 base(void){return (u64)lab_module_start-LAB_MAIN_TO_MODULE;}
static void zero(void *p,u32 n){for(u32 i=0;i<n;i++)((volatile u8 *)p)[i]=0;}
static void copy(void *p,const void *q,u32 n){
    for(u32 i=0;i<n;i++)((volatile u8 *)p)[i]=((const volatile u8 *)q)[i];
}
static void golden_policy_invalidate(void);
static void invalidate(void){
    golden_policy_invalidate();
    GState *s=&lab_golden_boundary_state;
    for(u32 i=0;i<12;i++){s->episode[i].active=0;__atomic_store_n(&s->mailbox[i].full,0,__ATOMIC_RELEASE);}
}

/* Exact verified native frames at3228 and3678. Walk only the recursive retail
 * traversal frames, ending at its optional-lock wrapper. Require a real lock,
 * its current parent linkage and RaceSystem's independently bound owner.
 * These are call-provenance reads of the current native stack, not stack scan.
 */
static u32 enclosing(u64 main,u64 manager,u64 system,u64 sp){
    if(!sp || (sp&15) || sp>=(1ull<<39)-0x2000 ||
       gq(sp,0x98)!=main+0x7f0ee4 || gq(sp,0xc8)!=main+0x7f0aac ||
       gq(sp,0xe8)!=main+0x60eec0 || gq(sp,0xd0)!=system+0xd0 ||
       gq(manager,0x28)!=system+0x178 || gq(manager,0x20)!=system+0x1b0 ||
       gq(system+0x178,0)!=main+0x12c33a8)return 0;
    u64 node=system+0xd0,frame=sp+0xf0;
    for(u32 depth=0;depth<32;depth++,frame+=0x20){
        u64 caller=gq(frame,0x18);
        if(caller==main+0x60ee78){
            u64 mutex=gq(node,0x78);
            return ptr(mutex) && mutex==gq(system,0x148);
        }
        if(caller!=main+0x60eed8)return 0;
        u64 parent=gq(frame,8);
        if(!ptr(parent) || gq(node,0x20)!=parent+0x20)return 0;
        node=parent;
    }
    return 0;
}

typedef struct {u64 magic;u32 abi,size;u64 sequence,generation,scene,system,race,group,scene_generation;
    u32 lock,active,fault,reserved;} GComLifetime;
extern GComLifetime lab_com_lifetime_record;
static u32 lifetime(GSnapshot *v){
    GComLifetime *r=&lab_com_lifetime_record;u64 seq=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
    if((seq&1) || r->magic!=0x31464c4d4f434148ull || r->abi!=2 || r->size!=88 ||
       r->active!=1 || r->lock || r->fault || r->reserved || !r->generation ||
       r->scene!=v->scene || r->system!=v->system || r->race!=v->race || r->group!=v->group)return 0;
    v->epoch=r->scene_generation;v->generation=r->generation;v->token=seq;
    return seq==__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
}
static u32 owner_same(const GOwner *a,const GOwner *z){
    return a->inventory==z->inventory && a->driver==z->driver && a->role==z->role && a->scale==z->scale;
}
static u32 scope_same(const GSnapshot *a,const GSnapshot *z){
    return a->scene==z->scene && a->system==z->system && a->race==z->race && a->group==z->group &&
        a->manager==z->manager && a->clock==z->clock && a->epoch==z->epoch &&
        a->generation==z->generation && a->token==z->token && a->thread==z->thread &&
        a->count==z->count && a->mode==z->mode;
}
static u32 collect(u64 main,u64 manager,u64 sp,GSnapshot *v){
    zero(v,sizeof(*v));v->base=main;v->manager=manager;
    QueueThreadSnapshot native;
    if(!lab_queue_thread_idle(main,&native) || !ptr(manager) || gq(manager,0)!=main+0x11a7188 ||
       !gb(manager,0x170))return 0;
    v->thread=native.tls;
    u64 embedded=gq(manager,0x28);if(!ptr(embedded))return 0;
    v->system=gq(embedded,0x30);
    if(!ptr(v->system) || embedded!=v->system+0x178 || gq(v->system,0)!=main+0x125a060 ||
       gw(v->system,0xb8)!=4 || gq(v->system,0x240)!=manager || !enclosing(main,manager,v->system,sp))return 0;
    u64 root=gq(main,0x12f6388);if(!ptr(root))return 0;
    u64 single=gq(root,0);if(!ptr(single))return 0;
    u64 link=gq(single,0x20);if(!ptr(link))return 0;
    v->scene=gq(link,8);
    if(!ptr(v->scene) || gq(v->scene,0)!=main+0x125e158 ||
       !(gb(v->scene,0x180)&4) || gq(v->scene,0x1b0)!=v->system)return 0;
    v->race=gq(v->system,0x238);if(!ptr(v->race)||gq(v->race,0)!=main+0x11b4308)return 0;
    v->group=gq(v->race,0xb0);if(!ptr(v->group)||gq(v->group,0x28)!=v->race)return 0;
    if(!lifetime(v))return 0;
    u64 clocks=gq(v->system,0x218);if(!ptr(clocks))return 0;
    v->clock=gq(clocks,0x50);if(!ptr(v->clock)||gw(v->clock,0x38)!=6)return 0;
    v->tick=gw(v->clock,0x44);if(v->tick>=0x80000000u)return 0;
    u64 config=((u64 (*)(void))(main+0x87c244))();
    if(!ptr(config)||gw(config,0)||gw(config,4)||gw(config,8)>1)return 0;
    v->mode=gw(config,8);
    ItemTaskRoster roster;
    if(!lab_item_task_collect(main,(void *)manager,(void *)v->race,&roster))return 0;
    v->count=roster.count;
    u64 fm=gq(v->group,0x68);if(!ptr(fm)||gq(fm,0x28)!=v->group)return 0;
    u32 nc=gw(fm,0x228);u64 wrappers=gq(fm,0xa0);
    if(nc>v->count||!ptr(wrappers))return 0;
    for(u32 i=0;i<v->count;i++){
        GOwner *o=&v->owners[i];o->inventory=roster.members[i].item;
        u64 ctx=roster.members[i].context,iw=roster.members[i].wrapper;
        o->driver=gq(ctx,0x18);o->role=gq(ctx,8);o->held=gq(o->inventory,0x58);
        if(!ptr(o->driver)||!ptr(o->role))return 0;
        u64 proxy=gq(o->driver,0x88),wrapper=gq(wrappers,8*i),order=gq(o->driver,0x70);
        if(!ptr(proxy)||gq(proxy,0x48)!=o->role || !ptr(wrapper)||gq(wrapper,8)!=o->driver ||
           !ptr(order)||gq(wrapper,0x18)!=order)return 0;
        u32 cpu=0;for(u32 k=0;k<nc;k++)cpu+=gq(fm,0xa8+16*k)==wrapper;
        if(cpu>1)return 0;
        if(!cpu || gb(o->role,0xd2)!=1 || gb(o->role,0xe7) || !gb(o->role,0xe1))continue;
        u32 n=gw(order,0x40);u64 rows=gq(order,0x48),rank_wrapper=gq(o->driver,0x78);
        if(!gb(order,0x65)||n!=v->count||i>=n||!ptr(rows))continue;
        if(rank_wrapper){
            if(!ptr(rank_wrapper)||gq(rank_wrapper,8)!=o->driver)continue;
            o->rank=gw(rank_wrapper,0x40);
        }else{
            u64 vt=gq(o->driver,0),pvt=gq(proxy,0);
            if(!ptr(vt)||!ptr(pvt)||gq(vt,0xe8)!=main+0x711b4||gq(pvt,0)!=main+0x140924)continue;
            o->rank=gb(rows,0x78*i+0x2a);
        }
        o->scale=gf(wrapper,0x320);o->progress=gf(rows,0x78*i+0x20)*o->scale;
        if(o->rank>=n||!finite(o->scale)||o->scale<=0||!finite(o->progress))continue;
        v->roster_mask|=1u<<i;
        if(gw(o->role,0x1cc)&0x01204018u)continue;
        o->golden=ptr(o->held) && (o->held==gq(o->inventory,0x60)||o->held==gq(o->inventory,0x68)) &&
            gq(o->held,0x70)==o->inventory && gq(o->held,0x78)==iw && gw(o->held,0x9c)==11;
        v->mask|=1u<<i;
    }
    return gw(v->clock,0x38)==6 && gw(v->clock,0x44)==v->tick && lifetime(v);
}
#include "golden_promotion_bridge_v305.h"
static void release_identity(void){
}
EXPORT void lab_golden_boundary_retire(void){
    GState *s=&lab_golden_boundary_state;
    if(s->scope.thread && s->scope.thread!=thread())return;
    u64 gate=__atomic_load_n(&s->gate,__ATOMIC_ACQUIRE);
    for(;;){
        u64 closing=PHASE(G_CLOSED)|(u32)gate;
        if(__atomic_compare_exchange_n(&s->gate,&gate,closing,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)){
            if((u32)closing){s->fault=1;return;}
            invalidate();release_identity();
            __atomic_store_n(&s->gate,0,__ATOMIC_RELEASE);return;
        }
    }
}
static void reduce(u32 i,const GSnapshot *end){
    GState *s=&lab_golden_boundary_state;GChannel *c=&s->channel[i];GEpisode *e=&s->episode[i];
    GMailbox *m=&s->mailbox[i];const GOwner *a=&s->scope.owners[i],*z=&end->owners[i];
    if(!(end->mask&(1u<<i)) || !(s->scope.mask&(1u<<i)) || !owner_same(a,z) ||
       __atomic_load_n(&c->fault,__ATOMIC_ACQUIRE)){
        e->active=0;__atomic_store_n(&m->full,0,__ATOMIC_RELEASE);return;
    }
    if(m->full && (end->tick<m->value.end_tick || end->tick-m->value.end_tick>120))m->full=0;
    if(e->active && (!c->count || end->tick<e->last_tick || end->tick-e->last_tick>2))e->active=0;
    for(u32 j=0;j<c->count;j++){
        const GScalar *r=&c->records[j];
        if(!r->timed || r->limited || r->duration!=450 || r->kind==G_LIMIT){e->active=0;continue;}
        if(r->kind==G_USE && r->witness && !r->previous && r->uses==1 && !r->elapsed &&
           z->golden && z->held==r->held){
            *e=(GEpisode){.first_sequence=r->sequence,.last_sequence=r->sequence,.held=r->held,
                .active=1,.start_tick=end->tick,.last_tick=end->tick,.start_rank=z->rank,.uses=1,
                .start_progress=z->progress,.scale=z->scale};continue;
        }
        if(!e->active)continue;
        if(r->sequence!=e->last_sequence+1 || r->held!=e->held || !a->golden || a->held!=e->held || e->scale!=z->scale){e->active=0;continue;}
        if(r->kind==G_USE){
            if(!r->witness || !z->golden || z->held!=e->held || r->previous!=e->uses ||
               r->uses!=r->previous+1 || r->elapsed!=e->elapsed || r->elapsed>=450){e->active=0;continue;}
            e->uses=r->uses;
        }else{
            if(r->uses!=e->uses || r->elapsed!=e->elapsed+1){e->active=0;continue;}
            if(r->kind==G_TICK){
                if(r->elapsed>=450 || !z->golden || z->held!=e->held){e->active=0;continue;}
                e->elapsed=r->elapsed;
            }else if(r->kind==G_EXPIRY && r->elapsed==450){
                if(!m->full){
                    m->value=(GoldenBoundaryEvidence){end->epoch,end->generation,z->driver,z->role,z->inventory,
                        e->first_sequence,r->sequence,i,e->start_tick,end->tick,e->start_rank,z->rank,e->uses,
                        GOLDEN_ITEM_EXIT_RANK_V1,0x3678,e->start_progress,z->progress};
                    __atomic_store_n(&m->full,1,__ATOMIC_RELEASE);
                }
                e->active=0;continue;
            }else{e->active=0;continue;}
        }
        e->last_sequence=r->sequence;e->last_tick=end->tick;
    }
}
EXPORT void lab_golden_boundary_dispatch(u32 event,const u64 *x,const void *native_sp){
    GState *s=&lab_golden_boundary_state;u64 main=base(),manager=x[19],sp=(u64)native_sp;
    if(event==1){
        if(s->fault || __atomic_load_n(&s->gate,__ATOMIC_ACQUIRE) || !lab_golden_native_ready(main))return;
        GSnapshot next;
        if(!collect(main,manager,sp,&next)){invalidate();release_identity();return;}
        if(s->serial==~0ull){s->fault=1;invalidate();release_identity();return;}
        if(!scope_same(&s->scope,&next) || next.tick<s->scope.tick)invalidate();
        for(u32 i=0;i<12;i++){
            if(!owner_same(&s->scope.owners[i],&next.owners[i])){s->episode[i].active=0;s->mailbox[i].full=0;}
            s->channel[i].count=0;__atomic_store_n(&s->channel[i].fault,0,__ATOMIC_RELEASE);
        }
        copy(&s->scope,&next,sizeof(next));s->serial++;s->command_tail=0;
        __atomic_store_n(&s->gate,PHASE(G_WORKERS),__ATOMIC_RELEASE);return;
    }
    if(s->scope.thread!=thread() || manager!=s->scope.manager)return;
    if(event==2){
        /* Native command dispatch also runs when this invocation's begin was
         * legitimately refused (intro, pause or results). No admitted scope
         * exists in that case. Preserve real active-phase/borrower faults. */
        if(!__atomic_load_n(&s->gate,__ATOMIC_ACQUIRE))return;
        QueueThreadSnapshot current;u64 expected=PHASE(G_WORKERS);
        if(lab_queue_thread_idle(main,&current) && current.tls==s->scope.thread &&
           enclosing(main,manager,s->scope.system,sp) &&
           __atomic_compare_exchange_n(&s->gate,&expected,PHASE(G_COMMANDS),0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))s->command_tail=1;
        else {s->fault=1;invalidate();}
        return;
    }
    if(event==3){
        u64 expected=PHASE(G_COMMANDS);
        if(!__atomic_compare_exchange_n(&s->gate,&expected,PHASE(G_CLOSED),0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)){
            lab_golden_boundary_retire();return;
        }
        GSnapshot end;
        if(!s->command_tail || !collect(main,manager,sp,&end) || !scope_same(&s->scope,&end) || end.tick!=s->scope.tick)invalidate();
        else {for(u32 i=0;i<end.count;i++)reduce(i,&end);golden_policy_commit(&end);}
        release_identity();__atomic_store_n(&s->gate,0,__ATOMIC_RELEASE);
    }
}
EXPORT void lab_golden_scalar_dispatch(u32 event,const u64 *x,const void *native_sp,u32 previous){
    if(event<4 || event>8)return;
    GState *s=&lab_golden_boundary_state;u32 phase=event==4||event==5?G_COMMANDS:G_WORKERS;
    u64 gate=__atomic_load_n(&s->gate,__ATOMIC_ACQUIRE);
    for(;;){
        if((gate>>32)!=phase || (u32)gate==~0u)return;
        if(__atomic_compare_exchange_n(&s->gate,&gate,gate+1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))break;
    }
    u64 inv=event==4?x[20]:x[19],sp=(u64)native_sp;u32 i=12;
    /* Inventory is borrowed from the exact native hook; bounds only validate
     * representation. Native graph dereferences never occur in this producer. */
    if(!ptr(inv) || !sp || (sp&15) || sp>=(1ull<<39)-0x1000)goto done;
    i=gw(inv,0x40);
    if(i>=s->scope.count || s->scope.owners[i].inventory!=inv)goto done;
    GChannel *c=&s->channel[i];u32 unlocked=0;
    if(!__atomic_compare_exchange_n(&c->lock,&unlocked,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        __atomic_store_n(&c->fault,1,__ATOMIC_RELEASE);goto done;
    }
    u32 witness=0;
    if(event==4){witness=x[11]==s->scope.base+0x41a7c && gq(sp,0x68)==s->scope.base+0x7954;}
    else if(event==5){witness=(u32)x[25]==1 && previous<0x7fffffffu &&
        (previous?x[10]==s->scope.base+0x41b64:x[10]==450) &&
        gq(sp,0x58)==s->scope.base+0x412c8 && gq(sp,0x98)==s->scope.base+0x5e2c &&
        gq(sp,0xd8)==s->scope.base+0x7954;}
    if((phase==G_COMMANDS && (thread()!=s->scope.thread || x[8]!=inv+0xf8 || previous>=0x7fffffffu ||
                            gw(inv,0xf8)!=previous+1)) ||
       (phase==G_WORKERS && gq(sp,0x28)!=s->scope.base+0x3edac) ||
       c->count>=GOLDEN_CAPTURE_CAPACITY || c->serial==~0ull){
        __atomic_store_n(&c->fault,1,__ATOMIC_RELEASE);goto unlock;
    }
    c->records[c->count++]=(GScalar){++c->serial,gq(inv,0x58),phase==G_COMMANDS?G_USE:event,
        gw(inv,0xf8),previous,gw(inv,0x104),gw(inv,0x108),gb(inv,0x100),gb(inv,0xf4),witness};
unlock:
    __atomic_store_n(&c->lock,0,__ATOMIC_RELEASE);
done:
    __atomic_fetch_sub(&s->gate,1,__ATOMIC_RELEASE);
}
EXPORT u32 lab_golden_boundary_take(u32 i,u64 epoch,u64 generation,u64 driver,
                                    u64 role,u64 inventory,u32 tick,GoldenBoundaryEvidence *out){
    GState *s=&lab_golden_boundary_state;
    if(i>=12 || !out || s->fault || s->scope.thread!=thread() || epoch!=s->scope.epoch || generation!=s->scope.generation)return 0;
    GMailbox *m=&s->mailbox[i];u32 expected=1;
    if(!__atomic_compare_exchange_n(&m->full,&expected,2,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
    GoldenBoundaryEvidence e=m->value;
    __atomic_store_n(&m->full,0,__ATOMIC_RELEASE);
    if(e.epoch!=epoch || e.generation!=generation || e.driver!=driver || e.role!=role || e.inventory!=inventory ||
       tick<e.end_tick || tick-e.end_tick>120)return 0;
    *out=e;return 1;
}
