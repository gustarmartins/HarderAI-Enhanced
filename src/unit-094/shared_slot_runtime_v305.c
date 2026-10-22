/* Private phase2 proposal. Exact native calc provenance supplies the existing
 * tree allocation pin; held identity admission/storage spans submit through
 * join. No policy or current-active-state re-admission is required for cleanup.
 * Not installable until paired production and route-transition qualification. */
#include "shared_slot_runtime_v305.h"
#include "queue_thread.h"
#include "director_pressure_native_v305.h"
extern char lab_module_start[];
extern u32 lab_shared_jobs_queue_boundary(u64,void *,QueueThreadSnapshot *);
extern u32 lab_shared_jobs_task_prerequisite(u64,void *);
#define SJ_EXPORT __attribute__((visibility("default")))
SJ_EXPORT const SharedSlotConfig lab_shared_slot_config={1,1,2700,1800,240,2,{0,0}};
SJ_EXPORT SharedSlotRecord lab_shared_slot_record;
/* prepare, provenance, lifetime, queue, changed, selected, applied, restored */
SJ_EXPORT u64 lab_shared_slot_counters[8];
static u64 sj_q(u64 p,u32 o){return *(const volatile u64 *)(p+o);}
static u32 sj_w(u64 p,u32 o){return *(const volatile u32 *)(p+o);}
static u8 sj_b(u64 p,u32 o){return *(const volatile u8 *)(p+o);}
static u32 ptr(u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x4000;}
static u64 thread(void){u64 t;__asm__ volatile("mrs %0,tpidrro_el0":"=r"(t));return t;}
static void count(u32 i){__atomic_fetch_add(&lab_shared_slot_counters[i],1,__ATOMIC_RELAXED);}
static u32 same(const void *a,const void *z,u32 n){
    for(u32 i=0;i<n;i++)if(((const u8 *)a)[i]!=((const u8 *)z)[i])return 0;return 1;
}
static void settings(SharedSlotConfig *out){
    const volatile u32 *in=(const volatile u32 *)&lab_shared_slot_config;
    for(u32 i=0;i<sizeof(*out)/4;i++)((u32 *)out)[i]=in[i];
}
static u32 mix(u32 x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
typedef struct {
    u64 scene,system,race,group,manager,clock,epoch,generation,token,queue,items,scheduler;
    u32 tick,queued_mask,front_witness,reserved;
} Scope;
typedef struct {
    u64 magic;u32 abi,size;
    u64 sequence,generation,scene,system,race,group,scene_generation;
    u32 lock,active,fault,reserved;
} ComLifetime;
extern ComLifetime lab_com_lifetime_record;
_Static_assert(sizeof(ComLifetime)==88,"COM lifetime ABI2");
static u32 life(Scope *s,SharedSlotJobFrame *f,u64 base){
    (void)f;(void)base;const ComLifetime *r=&lab_com_lifetime_record;
    u64 seq=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
    if(seq&1||r->magic!=0x31464c4d4f434148ull||r->abi!=2||r->size!=88||r->active!=1||r->lock||
       r->fault||r->reserved||!r->generation||r->scene!=s->scene||r->system!=s->system||
       r->race!=s->race||r->group!=s->group)return 0;
    s->epoch=r->generation;s->generation=r->generation;s->token=seq;
    return seq==__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
}
static u32 graph(u64 base,const void *context,u64 sp,Scope *s){
    u64 c=(u64)context,queue=sj_q(c,0x98),scheduler=sj_q(c,0xa0);
    if(sp!=c+0x320+LAB_SHARED_JOB_FRAME_BYTES||!ptr(queue)||queue<0x238||!ptr(scheduler)||
       sj_q(c,0xe8)!=sp+0x30||sj_q(c,0xf0)!=base+0x13e5b8||sj_q(sp,0x38)!=base+0x13dd00||
       sj_q(c,0)!=scheduler||sj_q(c,8)!=base+0xed80b6||sj_q(c,16)!=queue||sj_q(c,24)!=7||
       sj_q(c,32)!=sp+8||sj_q(c,40)!=sp)return 0;
    u64 race=queue-0x238;
    if(sj_q(sp,0x28)!=race||sj_q(race,0)!=base+0x11b4308||sj_w(race,0xc0)!=12||sj_b(race,0xe9)||sj_b(race,0xeb))return 0;
    QueueThreadSnapshot t;
    if(!lab_shared_jobs_queue_boundary(base,(void *)race,&t)||t.scheduler!=scheduler||
       !lab_shared_jobs_task_prerequisite(base,(void *)race))return 0;
    u64 group=sj_q(race,0xb0);if(!ptr(group)||sj_q(group,0)!=base+0x11ad808||sj_q(group,0x28)!=race||
       sj_w(group,0x58)!=12||sj_w(group,0x5c)!=12)return 0;
    u64 manager=sj_q(group,0x68);if(!ptr(manager)||sj_q(manager,0)!=base+0x11ae550||sj_q(manager,0x28)!=group||
       sj_w(manager,0x228)!=11||sj_w(manager,0x22c)!=1)return 0;
    u64 root=sj_q(base,0x12f6388);if(!ptr(root))return 0;
    u64 single=sj_q(root,0);if(!ptr(single))return 0;
    u64 link=sj_q(single,0x20);if(!ptr(link))return 0;
    u64 scene=sj_q(link,8);if(!ptr(scene)||sj_q(scene,0)!=base+0x125e158||!(sj_b(scene,0x180)&4))return 0;
    u64 system=sj_q(scene,0x1b0);
    if(!ptr(system)||sj_q(system,0)!=base+0x125a060||sj_q(system,0x238)!=race||sj_q(race,0x30)!=system||
       sj_q(race,0x28)!=system+0x178||sj_q(group,0x30)!=system)return 0;
    u64 clocks=sj_q(system,0x218);if(!ptr(clocks))return 0;
    u64 clock=sj_q(clocks,0x50);if(!ptr(clock)||sj_w(clock,0x38)!=6||sj_w(clock,0x44)>0x7ffff000u)return 0;
    u32 n=sj_w(queue,0xc0),cap=sj_w(queue,0xb0);u64 jobs=sj_q(queue,0xb8),drivers=sj_q(group,0x60),table=sj_q(manager,0xa0);
    if(n>12||cap<n||cap>32||!ptr(jobs)||!ptr(drivers)||!ptr(table))return 0;
    u32 queued=0,front_id=12;u64 front=sj_q(manager,0x80);
    if(!ptr(front)||sj_w(front,8)!=3)return 0;
    for(u32 i=0;i<12;i++){
        u64 wr=sj_q(table,8*i);if(!ptr(wr))return 0;
        if(sj_q(wr,0x10)==front&&sj_w(wr,0x24)==0){if(front_id!=12)return 0;front_id=i;}
    }
    if(front_id==12)return 0;
    for(u32 j=0;j<n;j++){
        u64 job=sj_q(jobs,8*j);if(!ptr(job))return 0;
        u64 driver=sj_q(job,0x20);if(!ptr(driver)||sj_q(job,0x28)!=base+0x70170||sj_q(driver,0x58)!=job)return 0;
        u32 id=12;for(u32 k=0;k<12;k++)if(sj_q(drivers,8*k)==driver)id=k;
        if(id==12||queued&(1u<<id)||sj_q(driver,0x60)!=group||sj_q(driver,0x28)!=group||sj_q(driver,0)!=base+0x11aea40)return 0;
        queued|=1u<<id;
    }
    *s=(Scope){.scene=scene,.system=system,.race=race,.group=group,.manager=manager,.clock=clock,
        .queue=queue,.items=jobs,.scheduler=scheduler,.tick=sj_w(clock,0x44),.queued_mask=queued,.front_witness=front_id};
    return 1;
}
static u32 mode(u64 base){
    u64 p=sj_q(base,0x12f6388);if(!ptr(p))return 0;p=sj_q(p,0);if(!ptr(p))return 0;
    p=sj_q(p,0x28);if(!ptr(p))return 0;p=sj_q(p,0x240);if(!ptr(p))return 0;p=sj_q(p,0x10);if(!ptr(p))return 0;
    return ((u64 (*)(void))(base+0x87c244))()==p&&!sj_w(p,0)&&!sj_w(p,4)&&sj_w(p,8)<=1;
}
static u32 collect(const Scope *s,u32 *available,u32 *quiet,u32 *slots){
    u64 table=sj_q(s->manager,0xa0),front=sj_q(s->manager,0x80),contexts=sj_q(s->race,0xc8);
    if(!ptr(contexts))return 0;*available=*quiet=0;
    for(u32 i=0;i<12;i++){
        u64 wr=sj_q(table,8*i);slots[i]=sj_w(wr,0x24);
        if(!(s->queued_mask&(1u<<i))||sj_q(wr,0x10)==front||slots[i]<3||slots[i]>=12||
           wr==sj_q(s->manager,0x168)||wr==sj_q(s->manager,0x170))continue;
        u64 driver=sj_q(wr,8),context=sj_q(contexts,8*i);if(!ptr(driver)||!ptr(context))return 0;
        u64 proxy=sj_q(driver,0x88);if(!ptr(proxy))return 0;u64 role=sj_q(proxy,0x48),physics=sj_q(proxy,0x50);
        PressureNativeActor a={role,physics,driver,wr,context,s->clock,i,s->tick};PressureNativeCapture c;
        if(!lab_pressure_native_collect(&a,&c))return 0;
        if(!c.active||c.remote)continue;
        *available|=1u<<i;
        u64 state=sj_q(driver,0x50);if(!ptr(state)||sj_q(state,0x88)!=driver)continue;
        u64 nav=sj_q(state,0xb0);if(!ptr(nav)||sj_q(nav,0x58)!=driver)continue;
        u64 producer=sj_q(nav,0x70);if(!ptr(producer)||sj_q(producer,0x70)!=driver)continue;
        /* No entry/expiry during an active physical or native planned maneuver.
         * Route transition safety still needs explicit qualification. */
        if(!c.sample.flags&&!sj_b(producer,0x41))*quiet|=1u<<i;
    }
    return 1;
}
static void fatal(void){__atomic_store_n(&lab_shared_slot_record.fault,1,__ATOMIC_RELEASE);__builtin_trap();}
SJ_EXPORT void lab_shared_jobs_prepare(const void *context,SharedSlotJobFrame *f,u64 sp){
    *f=(SharedSlotJobFrame){.magic=0x31424f4a534148ull,.native_sp=sp,.thread=thread()};
    /* Count EVERY invocation, including refused/off callbacks. A skipped
     * application means native jobs saw original labels, so a previous window
     * must not silently reenter SLOT00 during a newly active maneuver. */
    u64 invocation=__atomic_add_fetch(&lab_shared_slot_counters[0],1,__ATOMIC_RELAXED);
    if(!invocation)return;
    SharedSlotConfig config,final_config;settings(&config);const SharedSlotConfig *cfg=&config;
    if(cfg->abi!=1||cfg->enabled!=1||cfg->first_tick<1800||cfg->interval_ticks<600||
       cfg->duration_ticks<60||cfg->duration_ticks>=cfg->interval_ticks||!cfg->actors||cfg->actors>2||cfg->reserved[0]||cfg->reserved[1])return;
    u64 base=(u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if(!base||(base&4095)||base>=(1ull<<39)-0x1400000)return;
    Scope a={0},z={0};
    if(!graph(base,context,sp,&a)){count(1);return;}
    if(!life(&a,f,base)){count(2);return;}
    if(cfg->first_tick>0x7ffff000u||(u64)a.tick+cfg->interval_ticks>0x7ffff000u||
       (u64)a.tick+cfg->duration_ticks>0x7ffff000u||(u64)a.tick+60>0x7ffff000u){count(4);return;}
    if(!mode(base)||((int (*)(void))(base+0x7bf6c8))()!=(int)a.tick){count(4);return;}
    SharedSlotRecord *r=&lab_shared_slot_record;u32 expected=0;
    if(!__atomic_compare_exchange_n(&r->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;
    f->owned=1;
    if(r->fault||r->sequence&1||r->sequence>~0ull-2||r->updates==~0ull)return;
    SharedSlotRecord next=*r;next.lock=0;next.applied_mask=next.restored_mask=next.conflict_mask=0;
    if(r->epoch!=a.epoch||r->generation!=a.generation||r->group!=a.group||r->clock!=a.clock||a.tick<r->last_tick)
        next=(SharedSlotRecord){.sequence=r->sequence,.updates=r->updates,.epoch=a.epoch,.generation=a.generation,
            .group=a.group,.clock=a.clock,.last_tick=a.tick,.next_tick=a.tick<cfg->first_tick?cfg->first_tick:a.tick+cfg->interval_ticks};
    u32 available,quiet,slots[12];
    if(!collect(&a,&available,&quiet,slots)){count(3);return;}
    next.last_tick=a.tick;next.queued_mask=a.queued_mask;next.eligible_mask=quiet;
    u32 continuous=r->last_call==invocation-1?r->applied_mask:0;
    next.selected_mask&=available&(continuous|quiet);
    if(a.tick>=next.end_tick)next.selected_mask&=~quiet;
    if(!next.selected_mask&&a.tick>=next.next_tick){
        u32 unused=quiet&~next.visited_mask;
        if(!unused){unused=quiet;next.visited_mask=0;}
        u32 start=mix((u32)a.epoch^(u32)(a.epoch>>32)^next.cycle*0x9e3779b9u)%12,n=0;
        for(u32 k=0;k<12&&n<cfg->actors;k++){u32 id=(start+k)%12;if(unused&(1u<<id)){next.selected_mask|=1u<<id;n++;}}
        if(next.selected_mask){next.cycle++;next.visited_mask|=next.selected_mask;next.end_tick=a.tick+cfg->duration_ticks;
            next.next_tick=a.tick+cfg->interval_ticks;count(5);}
        else next.next_tick=a.tick+60;
    }
    u32 available2,quiet2,slots2[12];
    if(!mode(base)||!graph(base,context,sp,&z)||!life(&z,f,base)||!same(&a,&z,sizeof(a))||
       !collect(&z,&available2,&quiet2,slots2)||available!=available2||quiet!=quiet2||!same(slots,slots2,sizeof(slots))){count(4);return;}
    settings(&final_config);if(!same(cfg,&final_config,sizeof(config))){count(4);return;}
    __atomic_store_n(&r->sequence,next.sequence+1,__ATOMIC_RELEASE);f->scratch[0]=next.sequence+2;
    f->group=a.group;f->epoch=a.epoch;f->generation=a.generation;
    next.sequence++;next.lock=1;next.updates++;next.status=0;next.last_call=invocation;
    for(u32 i=0;i<12;i++)next.original_slots[i]=slots[i];
    if(next.selected_mask&&lab_shared_slot_begin(base,(void *)a.group,a.front_witness,next.selected_mask,a.epoch,a.generation,&f->slots)){
        next.applied_mask=next.selected_mask;next.applied_tick=a.tick;next.status=1;count(6);
    }
    *r=next;
}
SJ_EXPORT void lab_shared_jobs_finish(const void *context,SharedSlotJobFrame *f,u64 sp){
    (void)context;
    if(f->magic!=0x31424f4a534148ull||f->native_sp!=sp||f->thread!=thread())fatal();
    SharedSlotRecord *r=&lab_shared_slot_record;
    if(f->slots.active){
        u32 status=lab_shared_slot_end((void *)f->group,f->epoch,f->generation,&f->slots);
        /* Never return to native regrouping with an abandoned dirty receipt.
         * Different intervening slot values are preserved and reported. */
        if(!status||f->slots.active)fatal();
        r->restored_mask=f->slots.restored_mask;r->conflict_mask=f->slots.conflict_mask;
        r->status=status==1?2:3;count(7);
        if(status==2)r->selected_mask=0;
    }
    if(f->owned){
        if(f->scratch[0])__atomic_store_n(&r->sequence,f->scratch[0],__ATOMIC_RELEASE);
        __atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);f->owned=0;
    }
}
