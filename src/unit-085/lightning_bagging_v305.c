/* Private final-lap Lightning opportunity, v3.0.5.
 * The native picker owns the borrowed frame/curve. Its item mask, rank
 * multiplier, .01 cutoff, RNG, consumption and cooldowns execute unchanged.
 * Only a current local CPU in ranks9..12 may request a second evaluation of
 * this same curve at x=.3; retain the larger native curve result.
 */
typedef unsigned char u8;typedef unsigned short u16;typedef unsigned int u32;
typedef unsigned long long u64;
#define EXPORT __attribute__((visibility("default")))
extern const u8 lab_module_start[];
typedef union {u32 u;float f;} Bits;
typedef struct {u32 abi,enabled,count,minimum_rank;float minimum_progress,distance_floor;u32 delay_divisor,reserved;} Config;
EXPORT const Config lab_lightning_bagging_config={1,1,12,8,2.f,.3f,4,0};
/* Qualified scope, raised curve, unchanged curve, shortened first delay. */
EXPORT u64 lab_lightning_bagging_counts[4];
static u64 q(u64 p,u32 o){return *(const volatile u64 *)(p+o);}
static u32 w(u64 p,u32 o){return *(const volatile u32 *)(p+o);}
static u32 h(u64 p,u32 o){return *(const volatile u16 *)(p+o);}
static u32 b(u64 p,u32 o){return *(const volatile u8 *)(p+o);}
static float f(u64 p,u32 o){Bits n={.u=w(p,o)};return n.f;}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x3000;}
static u32 finite(float n){Bits v={.f=n};return (v.u&0x7fffffffu)<0x7f800000u;}
static u32 equal(const void *a,const void *z,u32 n){for(u32 i=0;i<n;i++)if(((const u8 *)a)[i]!=((const u8 *)z)[i])return 0;return 1;}
static void count(u32 n){__atomic_fetch_add(&lab_lightning_bagging_counts[n],1,__ATOMIC_RELAXED);}
static u64 main_base(void){return (u64)lab_module_start-LAB_MAIN_TO_MODULE;}

typedef struct {
    u64 root,single,service,holder,config,scene_link,scene,system,race,group,manager;
    u64 contexts,context,driver,proxy,role,wrappers,wrapper,clocks,clock,order,rows,row;
    u64 course_owner,table,course,reference,member[11];
    u64 item_component,roulette,inventories,inventory,item_proxy;
    u32 racer,tick,mode,count,rank,phase,flags,course_id;float raw,scale;
} View;

static u32 collect(u32 racer,View *v){
    for(u32 i=0;i<sizeof(*v);i++)((u8 *)v)[i]=0;
    u64 base=main_base();
    if((base&4095)||base>=(1ull<<39)-0x1400000||racer>=12)return 0;
    v->racer=racer;v->root=q(base,0x12f6388);if(!ptr(v->root))return 0;
    v->single=q(v->root,0);if(!ptr(v->single))return 0;
    v->service=q(v->single,0x28);if(!ptr(v->service))return 0;
    v->holder=q(v->service,0x240);if(!ptr(v->holder))return 0;
    v->config=q(v->holder,0x10);if(!ptr(v->config))return 0;
    v->mode=w(v->config,8);
    if(w(v->config,0)||w(v->config,4)||v->mode>1||w(v->config,0x18)||w(v->config,0x180)!=12)return 0;
    /* Owner-selected initial stock-course experiment. CourseID is the native
     * selector, not menu order; custom replacements inherit the selector.
     * Donut Plains3 remains tentative and is deliberately outside this list. */
    v->course_id=w(v->config,0x1a0);
    if(v->course_id!=42&&v->course_id!=56&&v->course_id!=62&&
       v->course_id!=100&&v->course_id!=112)return 0;
    v->scene_link=q(v->single,0x20);if(!ptr(v->scene_link))return 0;
    v->scene=q(v->scene_link,8);
    if(!ptr(v->scene)||q(v->scene,0)!=base+0x125e158||!(b(v->scene,0x180)&4))return 0;
    v->system=q(v->scene,0x1b0);
    if(!ptr(v->system)||q(v->system,0)!=base+0x125a060||w(v->system,0xb8)!=4)return 0;
    v->item_component=q(v->system,0x240);
    if(!ptr(v->item_component)||q(v->item_component,0)!=base+0x11a7188)return 0;
    v->roulette=q(v->item_component,0xf0);v->inventories=q(v->item_component,0xe8);
    if(!ptr(v->roulette)||!ptr(v->inventories))return 0;
    v->race=q(v->system,0x238);
    if(!ptr(v->race)||q(v->race,0)!=base+0x11b4308||w(v->race,0xc0)!=12||w(v->race,0x790)!=2)return 0;
    v->count=12;v->contexts=q(v->race,0xc8);v->group=q(v->race,0xb0);
    if(!ptr(v->contexts)||!ptr(v->group)||q(v->group,0)!=base+0x11ad808||q(v->group,0x28)!=v->race)return 0;
    v->manager=q(v->group,0x68);
    if(!ptr(v->manager)||q(v->manager,0)!=base+0x11ae550||q(v->manager,0x28)!=v->group||
       w(v->manager,0x228)!=11||w(v->manager,0x22c)!=1||w(v->manager,0x98)!=12)return 0;
    v->context=q(v->contexts,8*racer);if(!ptr(v->context))return 0;
    v->role=q(v->context,8);v->driver=q(v->context,0x18);
    if(!ptr(v->role)||!ptr(v->driver)||q(v->driver,0)!=base+0x11aea40||
       q(v->role,8)!=v->context||w(v->role,0xa8)!=racer||w(v->context,0x40)!=racer||
       q(v->driver,0x60)!=v->group||b(v->role,0xd2)!=1||b(v->role,0xd3)!=1||
       b(v->role,0xe1)!=1||b(v->role,0xe7))return 0;
    v->proxy=q(v->driver,0x88);
    if(!ptr(v->proxy)||q(v->proxy,0)!=base+0x11b44f8||q(v->proxy,0x48)!=v->role)return 0;
    v->inventory=q(v->inventories,8*racer);v->item_proxy=q(v->race,0x50+8*racer);
    if(!ptr(v->inventory)||!ptr(v->item_proxy)||q(v->driver,0x98)!=v->inventory||
       w(v->inventory,0x40)!=racer||q(v->inventory,0x48)!=v->item_proxy||q(v->item_proxy,0x48)!=v->role)return 0;
    v->wrappers=q(v->manager,0xa0);if(!ptr(v->wrappers))return 0;
    v->wrapper=q(v->wrappers,8*racer);v->reference=q(v->manager,0x170);
    if(!ptr(v->wrapper)||q(v->wrapper,8)!=v->driver||!ptr(v->reference)||v->wrapper==v->reference)return 0;
    u32 found=0;
    for(u32 i=0;i<11;i++){
        v->member[i]=q(v->manager,0xa8+16*i);
        if(!ptr(v->member[i])||v->member[i]==v->reference)return 0;
        for(u32 j=0;j<i;j++)if(v->member[i]==v->member[j])return 0;
        found+=v->member[i]==v->wrapper;
    }
    if(found!=1)return 0;
    v->clocks=q(v->system,0x218);if(!ptr(v->clocks))return 0;
    v->clock=q(v->clocks,0x50);v->order=q(v->clocks,0x58);
    if(!ptr(v->clock)||!ptr(v->order)||w(v->clock,0x38)!=6||q(v->driver,0x70)!=v->order)return 0;
    v->tick=w(v->clock,0x44);if(!v->tick||v->tick>=0x80000000u)return 0;
    v->course_owner=q(v->system,0x220);if(!ptr(v->course_owner))return 0;
    v->table=q(v->course_owner,0x190);if(!ptr(v->table)||w(v->table,0x14)!=3)return 0;
    v->course=q(v->table,0x100);
    if(!ptr(v->course)||q(v->order,0x38)!=v->course||w(v->order,0x40)!=12||
       w(v->order,0x60)!=12||b(v->order,0x64)!=3||b(v->order,0x65)!=1||b(v->course,0x32)!=3)return 0;
    v->rows=q(v->order,0x48);if(!ptr(v->rows))return 0;
    v->row=v->rows+0x78ull*racer;if(!ptr(v->row)||h(v->row,0x10)!=racer)return 0;
    v->rank=b(v->row,0x2a);v->phase=b(v->row,0x28);v->flags=h(v->row,0x12);
    v->raw=f(v->row,0x20);v->scale=f(v->course,0x2c);
    if(v->rank>=12||v->phase!=2||(v->flags&0x100)||!finite(v->raw)||v->raw<2.f||v->raw>=3.f||
       !finite(v->scale)||v->scale<=0)return 0;
    return w(v->clock,0x38)==6&&w(v->clock,0x44)==v->tick;
}

/* Called after the original curve evaluation. Context is its post-call
 * register snapshot; inputs[0/1/2] retain original curve/function/input bits.
 * Picker prologue stores its native racer argument in unused frame[0..3].
 */
EXPORT u32 lab_lightning_curve_finish(u64 *context,const u64 *inputs,const void *native_frame){
    Bits original={.u=w((u64)context,0x100)},input={.u=(u32)inputs[2]};
    u64 frame=(u64)native_frame,curve=inputs[0],evaluate=inputs[1],base=main_base();
    /* The displaced BLR originally exposed this return PC to its caller. */
    context[30]=base+0x46744;
    /* Retail splines can be negative before their first eligible range;
     * the native .01 cutoff owns that rejection, not a malformed-data guard. */
    if(!lab_lightning_bagging_config.enabled||context[23]!=10||!finite(original.f)||
       !finite(input.f)||input.f<0||input.f>=lab_lightning_bagging_config.distance_floor||
       !ptr(frame)||!ptr(curve)||q(curve,0)!=base+0x11b3c88||evaluate!=base+0x11dd2c||
       w(frame,0x14)!=10||!finite(f((u64)context,0x1b0))||f((u64)context,0x1b0)<=0)return original.u;
    View a,z;
    if(!collect(w(frame,0),&a)||a.rank<lab_lightning_bagging_config.minimum_rank||w(frame,0x1c)!=a.mode||
       q(frame,0x160)!=a.roulette||q(frame,0x178)!=base+0x45f08)return original.u;
    u64 knots=q(curve,8);u32 descriptor=w(curve,0x10);
    if(!ptr(knots))return original.u;
    count(0);
    float candidate=((float (*)(void *,float))evaluate)((void *)curve,lab_lightning_bagging_config.distance_floor);
    if(!collect(a.racer,&z)||!equal(&a,&z,sizeof(a))||q(curve,0)!=base+0x11b3c88||
       q(curve,8)!=knots||w(curve,0x10)!=descriptor||!finite(candidate)||candidate<=original.f){count(2);return original.u;}
    Bits result={.f=candidate};count(1);return result.u;
}

/* Runs at the original native delay store. Preserve its RNG draw and every
 * shorter native timer. Only held Lightning on a current final-lap CPU gets
 * 240..479 /4 =60..119 qualifying updates; no timer is reset during ticking.
 */
EXPORT u32 lab_lightning_first_delay(const void *planner,u32 original){
    if(!lab_lightning_bagging_config.enabled||original<240||original>479||!ptr((u64)planner))return original;
    u64 p=(u64)planner,driver=q(p,0x90),inventory=q(p,0xa8);
    if(!ptr(driver)||!ptr(inventory))return original;
    u64 state=q(driver,0x50),proxy=q(driver,0x88),held=q(inventory,0x58),item_proxy=q(inventory,0x48);
    if(!ptr(state)||q(state,0xa8)!=p||q(state,0x88)!=driver||!ptr(proxy)||!ptr(held)||
       q(driver,0x98)!=inventory||!ptr(item_proxy)||q(held,0x78)!=item_proxy||
       w(held,0x9c)!=10||q(held,0x70)!=inventory||
       (held!=q(inventory,0x60)&&held!=q(inventory,0x68)))return original;
    u64 role=q(proxy,0x48);if(!ptr(role))return original;
    View a,z;
    if(!collect(w(role,0xa8),&a)||a.driver!=driver||a.role!=role||a.inventory!=inventory||a.item_proxy!=item_proxy||
       w(p,0xb0)!=a.racer||w(inventory,0x40)!=a.racer||
       !collect(a.racer,&z)||!equal(&a,&z,sizeof(a))||q(driver,0x98)!=inventory||
       q(inventory,0x48)!=item_proxy||q(held,0x78)!=item_proxy||w(p,0xb0)!=a.racer||w(inventory,0x40)!=a.racer||
       q(inventory,0x58)!=held||w(held,0x9c)!=10||q(state,0xa8)!=p)return original;
    count(3);return original/lab_lightning_bagging_config.delay_divisor;
}
