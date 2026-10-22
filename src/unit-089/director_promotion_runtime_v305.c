/* Private post-manager native subgroup opportunities. No ORDER, RNG, tuning,
 * input, item or progress writes. Native front-label reassignment stays live.
 * The complete graph exchange happens only under a current phase-5 lease. */
#include "director_promotion_runtime_v305.h"
#include "main_task_lease.h"
#include "queue_thread.h"
extern char lab_module_start[];
extern u32 lab_promotion_queue_idle(u64,QueueThreadSnapshot *);
extern u32 lab_promotion_queue_boundary(u64,void *,QueueThreadSnapshot *);
extern u32 lab_promotion_task_try_lock(u64,void *,MainTaskLease *);
extern u32 lab_promotion_task_prerequisite(u64,void *);
extern void lab_promotion_task_unlock(MainTaskLease *);
#define DP_EXPORT __attribute__((visibility("default")))
DP_EXPORT const PromotionConfig lab_director_promotion_config={1,1,2400,1200,120,{0,0,0}};
DP_EXPORT PromotionRecord lab_director_promotion_record;
/* admitted visits, wrong graph, thread/lock, lifetime, mode/clock, changed,
 * exchange refusal, successful exchange. Counts are not rank gains. */
DP_EXPORT u64 lab_director_promotion_counters[8];
static u64 dpq(u64 p,u32 o){return *(const volatile u64 *)(p+o);}
static u32 dpw(u64 p,u32 o){return *(const volatile u32 *)(p+o);}
static u8 dpb(u64 p,u32 o){return *(const volatile u8 *)(p+o);}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x4000;}
static void count(u32 k){__atomic_fetch_add(&lab_director_promotion_counters[k],1,__ATOMIC_RELAXED);}
static u32 contains(u64 parent,u64 child){
    u32 n=dpw(parent,0x38),hits=0;u64 a=dpq(parent,0x40);
    if(!n||n>32||!ptr(a))return 0;
    for(u32 i=0;i<n;i++)hits+=dpq(a,8*i)==child;
    return hits==1;
}
typedef struct {
    u64 scene,system,race,group,manager,clock,epoch,generation,token;
    u32 tick,reserved;
} Scope;
typedef struct {
    u64 magic;u32 abi,size;
    u64 sequence,generation,scene,system,race,group,scene_generation;
    u32 lock,active,fault,reserved;
} ComLifetime;
extern ComLifetime lab_com_lifetime_record;
_Static_assert(sizeof(ComLifetime)==88,"COM lifetime ABI2");
static u32 life(u64 base,Scope *s
){
    (void)base;const ComLifetime *r=&lab_com_lifetime_record;
    u64 seq=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
    if(seq&1||r->magic!=0x31464c4d4f434148ull||r->abi!=2||r->size!=88||r->active!=1||
       r->lock||r->fault||r->reserved||!r->generation||r->scene!=s->scene||r->system!=s->system||
       r->race!=s->race||r->group!=s->group)return 0;
    s->epoch=r->generation;s->generation=r->generation;s->token=seq;
    return seq==__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
}
static u32 graph(u64 base,const void *context,Scope *s){
    u64 c=(u64)context,sp=c+0x320,g=dpq(c,0x98);
    if(!ptr(c)||!ptr(g)||dpq(c,0xf0)!=base+0x4d0bc||dpq(c,0xe8)!=sp+0x20||
       dpq(sp,0x28)!=base+0x4d0bc||dpq(g,0)!=base+0x11ad808)return 0;
    u64 race=dpq(g,0x28),manager=dpq(g,0x68),a=dpq(g,0x40);u32 n=dpw(g,0x38);
    if(!ptr(race)||!ptr(manager)||!ptr(a)||n!=14||dpq(c,0xa8)!=a+8||dpq(c,0xa0)!=n||
       dpq(a,0)!=manager||dpq(sp,0x18)!=race||dpq(race,0)!=base+0x11b4308||dpq(race,0xb0)!=g||
       dpq(manager,0)!=base+0x11ae550||dpq(manager,0x28)!=g||dpq(manager,0x30)||
       dpw(g,0x58)!=12||dpw(g,0x5c)!=12||dpw(race,0xc0)!=12||dpb(race,0xe9)||dpb(race,0xeb)||
       dpw(manager,0x228)!=11||dpw(manager,0x22c)!=1||!contains(g,manager)||!contains(race,g))return 0;
    u64 root=dpq(base,0x12f6388);if(!ptr(root))return 0;
    u64 single=dpq(root,0);if(!ptr(single))return 0;
    u64 link=dpq(single,0x20);if(!ptr(link))return 0;
    u64 scene=dpq(link,8);if(!ptr(scene)||dpq(scene,0)!=base+0x125e158||!(dpb(scene,0x180)&4))return 0;
    u64 system=dpq(scene,0x1b0);
    if(!ptr(system)||dpq(system,0)!=base+0x125a060||dpq(system,0x238)!=race||
       dpq(race,0x30)!=system||dpq(race,0x28)!=system+0x178||dpq(g,0x30)!=system)return 0;
    u64 clocks=dpq(system,0x218);if(!ptr(clocks))return 0;
    u64 clock=dpq(clocks,0x50);if(!ptr(clock)||dpw(clock,0x38)!=6||dpw(clock,0x44)>0x7fffffffu)return 0;
    s->scene=scene;s->system=system;s->race=race;s->group=g;s->manager=manager;s->clock=clock;
    s->tick=dpw(clock,0x44);return 1;
}
static u32 mode(u64 base){
    u64 p=dpq(base,0x12f6388);if(!ptr(p))return 0;
    p=dpq(p,0);if(!ptr(p))return 0;p=dpq(p,0x28);if(!ptr(p))return 0;
    p=dpq(p,0x240);if(!ptr(p))return 0;p=dpq(p,0x10);if(!ptr(p))return 0;
    return ((u64 (*)(void))(base+0x87c244))()==p&&!dpw(p,0)&&!dpw(p,4)&&dpw(p,8)<=1;
}
static u32 same(const void *a,const void *b_,u32 size){
    const u8 *x=a,*y=b_;for(u32 i=0;i<size;i++)if(x[i]!=y[i])return 0;return 1;
}
static u32 mix(u32 x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
static u32 candidates(const Scope *s,u32 *mask){
    u64 table=dpq(s->manager,0xa0);if(!ptr(table))return 0;
    u64 front=dpq(s->manager,0x80);if(!ptr(front)||dpw(front,8)!=3)return 0;
    u32 eligible=0;
    for(u32 j=0;j<11;j++){
        u64 wr=dpq(s->manager,0xb0+16*j);if(!ptr(wr)||dpw(wr,0x20)!=j)return 0;
        u32 id=12;for(u32 k=0;k<12;k++)if(dpq(table,8*k)==wr)id=k;
        if(id==12)return 0;
        if(dpq(wr,0x10)==front||dpw(wr,0x24)<3)continue;
        u64 d=dpq(wr,8);if(!ptr(d))return 0;
        u64 proxy=dpq(d,0x88);if(!ptr(proxy))return 0;
        u64 role=dpq(proxy,0x48);if(!ptr(role)||dpw(role,0xa8)!=id)return 0;
        if(dpw(role,0x1cc)&0x01204018u||!dpb(role,0xe1)||dpb(role,0xe7))continue;
        eligible|=1u<<id;
    }
    *mask=eligible;return 1;
}
DP_EXPORT void lab_director_promotion_dispatch(const void *context){
    const PromotionConfig *cfg=&lab_director_promotion_config;
    if(cfg->abi!=1||cfg->enabled!=1||cfg->first_tick<1800||cfg->interval_ticks<600||
       !cfg->retry_ticks||cfg->retry_ticks>cfg->interval_ticks||cfg->reserved[0]||cfg->reserved[1]||cfg->reserved[2])return;
    u64 base=(u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if(!base||(base&4095)||base>=(1ull<<39)-0x1400000)return;
    PromotionRecord *r=&lab_director_promotion_record;u32 expected=0;
    if(!__atomic_compare_exchange_n(&r->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;
    MainTaskLease lease={0};Scope a={0},z={0};QueueThreadSnapshot initial,last;
    PromotionRecord next={0};u32 publish=0;
    if(r->fault||r->sequence&1||r->sequence>~0ull-2||r->updates==~0ull)goto done;
    if(!lab_promotion_queue_idle(base,&initial)){count(2);goto done;}
    if(!graph(base,context,&a)){count(1);goto done;}
    if(!lab_promotion_task_try_lock(base,(void *)a.race,&lease)||
       !lab_promotion_queue_boundary(base,(void *)a.race,&last)||
       initial.tls!=last.tls||initial.nn_thread!=last.nn_thread||initial.current_thread!=last.current_thread||
       initial.scheduler!=last.scheduler){count(2);goto done;}
    if(!life(base,&a
    )){count(3);goto done;}
    if(!mode(base)||((int (*)(void))(base+0x7bf6c8))()!=(int)a.tick){count(4);goto done;}
    count(0);next=*r;next.lock=0;
    if(r->epoch!=a.epoch||r->generation!=a.generation||r->group!=a.group||r->clock!=a.clock||a.tick<r->last_tick){
        next=(PromotionRecord){.sequence=r->sequence,.updates=r->updates,.epoch=a.epoch,.generation=a.generation,
            .group=a.group,.clock=a.clock,.last_tick=a.tick,
            .next_tick=a.tick<cfg->first_tick?cfg->first_tick:a.tick+cfg->interval_ticks};
        publish=1;goto verify;
    }
    if(a.tick==r->last_tick)goto done;
    next.last_tick=a.tick;publish=1;
    if(a.tick<next.next_tick)goto verify;
    u32 eligible;
    if(!candidates(&a,&eligible)){count(1);goto done;}
    next.eligible_mask=eligible;next.status=0;
    if(!eligible){next.next_tick=a.tick+cfg->retry_ticks;goto verify;}
    u32 unused=eligible&~next.visited_mask;
    if(!unused){unused=eligible;next.visited_mask=0;}
    u32 start=mix((u32)a.epoch^(u32)(a.epoch>>32)^next.cycle*0x9e3779b9u)%12,chosen=12;
    /* Rotate opportunities among qualified midfield/tail racers. This does
     * not consume the native RNG or repeatedly select the same actor. */
    for(u32 k=0;k<12;k++){u32 id=(start+k)%12;if(unused&(1u<<id)){chosen=id;break;}}
    next.result.source_racer=chosen;next.result.target_slot=next.cycle%3;
    next.status=3; /* intent; only becomes EXCHANGED after exact native stores */
verify:
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    if(!mode(base)||!lab_promotion_task_prerequisite(base,(void *)a.race)||
       !lab_promotion_queue_boundary(base,(void *)a.race,&initial)||!same(&initial,&last,sizeof(initial))||
       !graph(base,context,&z)||!life(base,&z
       )||!same(&a,&z,sizeof(a))){count(5);goto done;}
    if(next.status==3){
        u32 chosen=next.result.source_racer,target=next.result.target_slot;
        u32 outcome=lab_director_promotion_exchange_v305(base,(void *)a.group,chosen,target,&next.result);
        next.status=outcome;
        if(outcome==DX_EXCHANGED){
            next.visited_mask|=1u<<chosen;next.cycle++;next.exchange_tick=a.tick;
            next.next_tick=a.tick+cfg->interval_ticks;count(7);
        }else{next.next_tick=a.tick+cfg->retry_ticks;count(6);}
    }
    if(publish){
        __atomic_add_fetch(&r->sequence,1,__ATOMIC_ACQ_REL);
        next.sequence=r->sequence;next.updates++;
        /* The private lock and publication sequence are owned separately. */
        for(u32 k=16;k<sizeof(*r);k++)((volatile u8 *)r)[k]=((const u8 *)&next)[k];
        __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
    }
done:
    lab_promotion_task_unlock(&lease);
    __atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);
}
