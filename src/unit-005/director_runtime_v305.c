/* Private alpha: read qualified native geometry and replace this CPU's
 * pre-limit sum. No game-object stores, sorting, native RNG or item inference.
 * Lifetime receipts protect epoch identity; native callback/scheduler ownership
 * supplies allocation lifetime. Repeated collection detects changing inputs,
 * but does not claim atomic live-tuner or complete-pack publication. */
#include "director_runtime_v305.h"
#include "golden_promotion_publication_v305.h"
extern GoldenPromotionPublication lab_golden_promotion_publication;
extern char lab_module_start[];
typedef unsigned char dr_u8;
typedef union {dc_u32 u;float f;} FloatBits;
DR_EXPORT const DirectorRuntimeConfig lab_director_config={
    0x315249444941484full,1,sizeof(DirectorRuntimeRecord),12,1,0x6d6bc,0x6d6c0,
    {1,1,120,0,1.f,1.f,240.f,54.f,.2f,.1f}};
DR_EXPORT DirectorRuntimeRecord lab_director_records[12];
/* Attempt, owner refusal, mode refusal, lifetime refusal, contention,
 * changed sample, invalid policy and accepted publication. Module lifetime. */
DR_EXPORT dc_u64 lab_director_counters[8];

static dc_u64 dr_q(dc_u64 p,dc_u32 o){return *(const volatile dc_u64 *)(p+o);}
static dc_u32 dr_w(dc_u64 p,dc_u32 o){return *(const volatile dc_u32 *)(p+o);}
static dr_u8 dr_b(dc_u64 p,dc_u32 o){return *(const volatile dr_u8 *)(p+o);}
static float dr_f(dc_u64 p,dc_u32 o){FloatBits v={.u=dr_w(p,o)};return v.f;}
static dc_u32 dr_ptr(dc_u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x3000;}
static dc_u32 dr_finite(float x){FloatBits v={.f=x};return (v.u&0x7fffffffu)<0x7f800000u;}
/* Private snapshots and writer-owned records only. Keep explicit volatile
 * stores without the old byte-at-a-time cost on expanded telemetry records. */
static void dr_zero(void *p,dc_u32 n){
    dc_u32 k=0;
    if(!((dc_u64)p&7))for(;n-k>=8;k+=8)*(volatile dc_u64 *)((dr_u8 *)p+k)=0;
    for(;k<n;k++)((volatile dr_u8 *)p)[k]=0;
}
static void dr_copy(void *p,const void *s,dc_u32 n){
    dc_u32 k=0;
    if(!(((dc_u64)p|(dc_u64)s)&7))for(;n-k>=8;k+=8)
        *(volatile dc_u64 *)((dr_u8 *)p+k)=*(const dc_u64 *)((const dr_u8 *)s+k);
    for(;k<n;k++)((volatile dr_u8 *)p)[k]=((const dr_u8 *)s)[k];
}
static dc_u32 dr_equal(const void *a,const void *z,dc_u32 n){
    dc_u32 k=0;
    if(!(((dc_u64)a|(dc_u64)z)&7))for(;n-k>=8;k+=8)
        if(*(const dc_u64 *)((const dr_u8 *)a+k)!=*(const dc_u64 *)((const dr_u8 *)z+k))return 0;
    for(;k<n;k++)if(((const dr_u8 *)a)[k]!=((const dr_u8 *)z)[k])return 0;
    return 1;
}
static dc_u64 dr_hashword(dc_u64 h,dc_u64 word){return (h^word)*1099511628211ull;}
static void dr_count(dc_u32 k){__atomic_fetch_add(&lab_director_counters[k],1,__ATOMIC_RELAXED);}

/* Private native progress observation inside the existing qualified callback.
 * All reads are repeated by dr_collect's before/after Snapshot comparison.
 * Boundaries come from the current order/course graph; never modulo distance.
 * The descriptor occupies the previously-zero private input/state reserve.
 * No game writes. Ordinary/section progress can be unavailable independently
 * of a valid race. No-reference races retain the established timed schedule. */
DR_EXPORT const dc_u32 lab_director_progress_config[6]={1,672,456,72,1,127};
typedef struct {
    dc_u64 owner,table,course,order,rows,wrapper,boundaries,groups,nodes[4];
    dc_u32 lap_number,count,reference,phase,flags,boundary_count,group_count;
    dc_u32 group_index[4],node_index[4],node_count[4];
    float raw,scale,endpoints[4];
} DirectorProgressContext;
static dc_u32 dp_h(dc_u64 p,dc_u32 off){return *(const volatile unsigned short *)(p+off);}
static dc_u32 dp_endpoint(DirectorProgressContext *p,dc_u32 index,dc_u32 out){
    if(p->lap_number==1&&index==p->count){p->endpoints[out]=1.f;return 1;}
    if(index>=p->boundary_count)return 0;
    dc_u32 pair=dr_w(p->boundaries,4*index),group=pair&65535u,node=pair>>16;
    if(group>=p->group_count)return 0;
    dc_u64 g=p->groups+0x70ull*group;
    if(!dr_ptr(g))return 0;
    dc_u32 count=dr_w(g,0x50);dc_u64 array=dr_q(g,0x58);
    if(!count||count>65535||node>=count||!dr_ptr(array))return 0;
    dc_u64 point=array+0x790ull*node;
    if(!dr_ptr(point)||dr_w(point,0x60)!=pair)return 0;
    float value=dr_f(point,0x6c);if(!dr_finite(value))return 0;
    p->nodes[out]=point;p->group_index[out]=group;p->node_index[out]=node;
    p->node_count[out]=count;p->endpoints[out]=value;return 1;
}
/* 0 invalid native graph; 1 racing progress; 2 before start / finished. */
static dc_u32 dp_collect(dc_u64 system,dc_u64 clocks,dc_u64 wrapper,
    dc_u32 racer,dc_u32 racers,DirectorProgressContext *p,dc_u32 *descriptor,dc_u32 *units_bits){
    p->wrapper=wrapper;p->reference=racer;
    p->owner=dr_q(system,0x220);p->order=dr_q(clocks,0x58);
    if(!dr_ptr(p->owner)||!dr_ptr(p->order))return 0;
    p->table=dr_q(p->owner,0x190);if(!dr_ptr(p->table))return 0;
    p->lap_number=dr_w(p->table,0x14);p->course=dr_q(p->table,0x100);
    if(p->lap_number>127||!dr_ptr(p->course)||dr_q(p->order,0x38)!=p->course||
       dr_q(wrapper,0x18)!=p->order||dr_w(p->order,0x40)!=racers||
       dr_w(p->order,0x60)!=racers||dr_b(p->order,0x65)!=1)return 0;
    p->count=dr_b(p->order,0x64);
    if(!p->count||p->count>127||dr_b(p->course,0x32)!=p->count)return 0;
    p->rows=dr_q(p->order,0x48);if(!dr_ptr(p->rows))return 0;
    dc_u64 row=p->rows+0x78ull*racer;
    if(!dr_ptr(row)||dp_h(row,0x10)!=racer)return 0;
    p->raw=dr_f(row,0x20);p->phase=dr_b(row,0x28);p->flags=dp_h(row,0x12);
    p->scale=dr_f(p->course,0x2c);
    if(!dr_finite(p->raw)||!dr_finite(p->scale)||p->scale<=0||dr_f(wrapper,0x320)!=p->scale)return 0;
    if(p->lap_number>=2&&p->count!=p->lap_number)return 0;
    if(p->phase==255||(p->flags&0x100)||p->phase==p->count)return 2;
    if(p->phase>=p->count)return 0;
    float fraction,units;
    if(p->lap_number>=2){
        fraction=p->raw-(float)p->phase;units=p->raw;
    }else{
        p->boundary_count=dr_w(p->course,0x40);p->boundaries=dr_q(p->course,0x48);
        p->group_count=dr_w(p->course,0x10);p->groups=dr_q(p->course,0x20);
        if(p->boundary_count!=p->count+(p->lap_number==0)||!dr_ptr(p->boundaries)||
           !p->group_count||p->group_count>4096||!dr_ptr(p->groups))return 0;
        if(!dp_endpoint(p,0,0)||!dp_endpoint(p,p->count,1)||
           !dp_endpoint(p,p->phase,2)||!dp_endpoint(p,p->phase+1,3))return 0;
        float first=p->endpoints[0],last=p->endpoints[1],lo=p->endpoints[2],hi=p->endpoints[3];
        if(!(first<last&&lo<hi&&first<=lo&&hi<=last)||!dr_finite(hi-lo))return 0;
        fraction=(p->raw-lo)/(hi-lo);units=(float)p->phase+fraction;
    }
    if(!dr_finite(fraction)||fraction<0||fraction>1||!dr_finite(units)||units<0)return 0;
    if(units==(float)p->count)return 2;
    if(units>(float)p->count)return 0;
    FloatBits bits={.f=units};
    *descriptor=(p->lap_number>=2?1u:2u)|(p->count<<8)|((racer+1)<<16);
    *units_bits=bits.u;return 1;
}

#include "director_opening_runtime_v305.h"
DR_EXPORT const DirectorOpeningConfig lab_director_play_defaults={1,1,1200,600,2,{0,0,0}};
DR_EXPORT dc_u32 lab_director_play_request(DirectorOpeningContext *out,dc_u64 *sequence){
    if(!do_request(out,sequence))return 0;
    if(!*sequence)out->config=lab_director_play_defaults;
    return 1;
}

#include "director_ladder_runtime_v305.h"
#include "director_pressure_runtime_v305.h"
typedef struct {
    DirectorProgressContext progress;
    DirectorPressureContext pressure;
    DirectorLadderContext ladder;
    DirectorOpeningContext opening;dc_u64 opening_sequence;
    dc_u64 controller,scene,system,race,group,manager,parameters,clock;
    dc_u64 lifetime_token,lifetime_generation,golden_role,golden_inventory;
    DirectorCoreInput policy;
} Snapshot;

typedef struct {
    dc_u64 magic;dc_u32 abi,size;
    dc_u64 sequence,generation,scene,system,race,group,scene_generation;
    dc_u32 lock,active,fault,reserved;
} DirectorComLifetime;
extern DirectorComLifetime lab_com_lifetime_record;
_Static_assert(sizeof(DirectorComLifetime)==88,"COM lifetime ABI2");

static dc_u32 dr_lifetime(Snapshot *s){
    const DirectorComLifetime *r=&lab_com_lifetime_record;
    dc_u64 seq=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
    if((seq&1)||r->magic!=0x31464c4d4f434148ull||r->abi!=2||r->size!=88||
       r->active!=1||r->fault||r->lock||r->reserved||!r->generation||
       r->scene!=s->scene||r->system!=s->system||r->race!=s->race||r->group!=s->group)return 0;
    s->lifetime_token=seq;s->lifetime_generation=r->generation;s->policy.epoch=r->generation;
    /* Contention and counter exhaustion can poison this receipt without
     * advancing sequence. Re-read these atomic flags after the final sequence
     * observation; matching sequence alone cannot establish a live receipt.
     * Native callback ownership still supplies allocation lifetime. */
    return seq==__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE)&&
           !__atomic_load_n(&r->fault,__ATOMIC_ACQUIRE)&&
           !__atomic_load_n(&r->lock,__ATOMIC_ACQUIRE)&&
           __atomic_load_n(&r->active,__ATOMIC_ACQUIRE)==1;
}

static dc_u32 dr_collect(dc_u64 ctrl,dc_u64 base,Snapshot *s){
    dr_zero(s,sizeof(*s));s->controller=ctrl;
    if(!lab_director_play_request(&s->opening,&s->opening_sequence)||!pr_begin(&s->pressure))return 0;
    if(!dr_ptr(ctrl)||dr_q(ctrl,0)!=base+0x11ae6e8)return 0;
    dc_u64 state=dr_q(ctrl,0x28),group=dr_q(ctrl,0x38),manager=dr_q(ctrl,0x40),
        wrapper=dr_q(ctrl,0x48),params=dr_q(ctrl,0x50),driver=dr_q(ctrl,0x58);
    if(!dr_ptr(state)||!dr_ptr(group)||!dr_ptr(manager)||!dr_ptr(wrapper)||!dr_ptr(params)||!dr_ptr(driver)||
       dr_q(state,0)!=base+0x11ae810||dr_q(group,0)!=base+0x11ad808||
       dr_q(manager,0)!=base+0x11ae550||dr_q(driver,0)!=base+0x11aea40||
       dr_q(driver,0x28)!=group||dr_q(driver,0x60)!=group||dr_q(driver,0x50)!=state||
       dr_q(state,0x28)!=driver||dr_q(state,0x88)!=driver||dr_q(state,0x90)!=ctrl||
       dr_q(state,0xa0)!=ctrl||dr_q(group,0x68)!=manager||dr_q(manager,0x28)!=group||
       dr_q(manager,0x78)!=params||dr_q(wrapper,8)!=driver)return 0;
    dc_u64 param_owner=dr_q(group,0x70);
    if(!dr_ptr(param_owner)||dr_q(param_owner,0x18)!=params)return 0;
    dc_u64 root=dr_q(base,0x12f6388);if(!dr_ptr(root))return 0;
    dc_u64 single=dr_q(root,0);if(!dr_ptr(single))return 0;
    dc_u64 link=dr_q(single,0x20);if(!dr_ptr(link))return 0;
    dc_u64 scene=dr_q(link,8);if(!dr_ptr(scene)||dr_q(scene,0)!=base+0x125e158||!(dr_b(scene,0x180)&4))return 0;
    dc_u64 system=dr_q(scene,0x1b0);if(!dr_ptr(system)||dr_q(system,0)!=base+0x125a060)return 0;
    dc_u64 race=dr_q(system,0x238),clocks=dr_q(system,0x218);
    if(!dr_ptr(race)||!dr_ptr(clocks)||dr_q(race,0)!=base+0x11b4308||dr_q(race,0xb0)!=group||dr_q(group,0x28)!=race)return 0;
    dc_u64 clock=dr_q(clocks,0x50);if(!dr_ptr(clock)||dr_w(clock,0x38)!=6||dr_w(race,0x790)!=2)return 0;
    if(do_requested(&s->opening)){
        dc_u64 clock_type=dr_q(clock,0);
        if(clock_type!=base+0x125a618&&clock_type!=base+0x125a828)return 0;
    }
    dc_u32 n=dr_w(group,0x58),nc=dr_w(manager,0x228),nh=dr_w(manager,0x22c);
    if(!n||n>12||!nc||nc>n||nh>n||nc+nh!=n||dr_w(manager,0x98)!=n||dr_w(race,0xc0)!=n)return 0;
    dc_u64 drivers=dr_q(group,0x60),wrappers=dr_q(manager,0xa0),contexts=dr_q(race,0xc8);
    if(!dr_ptr(drivers)||!dr_ptr(wrappers)||!dr_ptr(contexts))return 0;
    s->scene=scene;s->system=system;s->race=race;s->group=group;s->manager=manager;
    s->parameters=params;s->clock=clock;
    if(!dr_lifetime(s))return 2;
    DirectorCoreInput *in=&s->policy;
    in->tick=dr_w(clock,0x44);in->count=nc;
    dc_u32 cpu_ids=0,human_ids=0,found=0,used=0;
    /* Native manager membership owns the CPU/human partition. */
    for(dc_u32 j=0;j<nc;j++){
        dc_u64 wr=dr_q(manager,0xa8+16*j);dc_u32 match=0;
        for(dc_u32 k=0;k<n;k++)if(dr_q(wrappers,8*k)==wr)match=1u<<k;
        if(!match||(cpu_ids&match))return 0;cpu_ids|=match;
    }
    for(dc_u32 j=0;j<nh;j++){
        dc_u64 wr=dr_q(manager,0x168+16*j);dc_u32 match=0;
        for(dc_u32 k=0;k<n;k++)if(dr_q(wrappers,8*k)==wr)match=1u<<k;
        if(!match||((cpu_ids|human_ids)&match))return 0;human_ids|=match;
    }
    if((cpu_ids|human_ids)!=(1u<<n)-1)return 0;
    dc_u64 reference=nh?dr_q(manager,0x170):0;
    s->pressure.count=n;s->pressure.cpu_mask=cpu_ids;
    for(dc_u32 j=0;j<n;j++){
        dc_u64 d=dr_q(drivers,8*j),wr=dr_q(wrappers,8*j),ctx=dr_q(contexts,8*j);
        if(!dr_ptr(d)||!dr_ptr(wr)||!dr_ptr(ctx)||dr_q(d,0)!=base+0x11aea40||
           dr_q(d,0x28)!=group||dr_q(d,0x60)!=group||dr_q(wr,8)!=d||dr_q(ctx,0x18)!=d||dr_w(ctx,0x40)!=j)return 0;
        dc_u64 proxy=dr_q(d,0x88),role=dr_q(ctx,8);
        if(!dr_ptr(proxy)||!dr_ptr(role)||dr_q(proxy,0)!=base+0x11b44f8||dr_q(proxy,0x48)!=role||
           dr_q(role,8)!=ctx||dr_w(role,0xa8)!=j||!dr_b(role,0xe1)||dr_b(role,0xe7))return 0;
        dc_u64 physics=dr_q(role,0x28);
        if(!dr_ptr(physics)||dr_q(physics,0xf8)!=role||dr_q(proxy,0x50)!=physics)return 0;
        for(dc_u32 k=0;k<j;k++)if(dr_q(drivers,8*k)==d||dr_q(wrappers,8*k)==wr||dr_q(contexts,8*k)==ctx)return 0;
        float progress=dr_f(wr,0x2c);if(!dr_finite(progress))return 0;
        if(s->pressure.enabled){
            PressureNativeActor actor={role,physics,d,wr,ctx,clock,j,in->tick};
            if(!lab_pressure_native_collect(&actor,&s->pressure.actors[j]))return 0;
            if(wr==reference)s->pressure.reference=j;
        }
        dc_u32 cpu=(cpu_ids&(1u<<j))!=0;
        if(cpu){
            dc_u32 slot=dr_w(wr,0x24);if(used>=nc||slot>=12)return 0;
            in->peers[used++]=(DirectorCorePeer){d,progress,j,slot,0};
        }
        if(wr==reference){
            if(cpu)return 0;
            float speed=dr_f(physics,0x37c);if(!dr_finite(speed))return 0;
            in->human_valid=1;in->human_progress=progress;
            in->human_weight=speed<=0?0:speed>=.33333334f?1:speed*3.f;
            dc_u32 progress_state=dp_collect(system,clocks,wr,j,n,&s->progress,
                &in->reserved[0],&in->reserved[1]);
            if(!progress_state)return 0;
            if(progress_state==2)in->flags|=DC_UNAVAILABLE;
        }
        if(d==driver){
            if(!cpu||wr!=wrapper||dr_b(role,0xd2)!=1||dr_b(role,0xd3)!=1)return 0;
            dc_u32 slot=dr_w(wr,0x24);if(slot>=12)return 0;
            in->racer=j;in->owner=d;found++;
            s->golden_role=role;s->golden_inventory=dr_q(d,0x98);
            in->native_rung=dr_f(params,0x14+4*(slot?slot:1));
            s->opening.capped_rung=in->native_rung;
            if(do_requested(&s->opening)&&slot>s->opening.config.slot_cap){
                dc_u32 cap=s->opening.config.slot_cap;
                s->opening.capped_rung=dr_f(params,0x14+4*(cap?cap:1));
            }
            if(dr_w(role,0x1cc)&0x01204018u)in->flags|=DC_UNAVAILABLE;
            dc_u64 dash=dr_q(physics,0x118);
            if(!dr_ptr(dash)||dr_q(dash,8)!=role)return 0;
            if((dr_b(dash,0x10)&&((int)dr_w(dash,0x14)>0))||(int)dr_w(role,0x1e4)>0)in->flags|=DC_BOOST;
        }
    }
    if(found!=1||used!=nc||((nh!=0)!=in->human_valid))return 0;
    /* Active FCRA geometry is kept; catch windows use their saved authored
     * values so FCRA cannot unexpectedly change catch eligibility. */
    for(dc_u32 j=0;j<12;j++)in->targets[j]=dr_f(params,0x44+4*j);
    in->up=dr_f(params,0x74);in->down=dr_f(params,0x78);in->cu_ratio=dr_f(params,0x84);in->cd_ratio=dr_f(params,0x90);
    in->cu_min=dr_f(params,0xf4);in->cu_max=dr_f(params,0xf8);in->cd_min=dr_f(params,0xfc);in->cd_max=dr_f(params,0x100);
    dc_u64 hash=1469598103934665603ull;
    for(dc_u32 off=0;off<=0x10;off+=4)hash=dr_hashword(hash,dr_w(params,off));
    for(dc_u32 off=0x74;off<=0x90;off+=4)if(off!=0x7c&&off!=0x80&&off!=0x88&&off!=0x8c)hash=dr_hashword(hash,dr_w(params,off));
    for(dc_u32 off=0x94;off<=0x100;off+=4)hash=dr_hashword(hash,dr_w(params,off));
    in->tune_revision=hash?hash:1;
    /* Epoch is a qualified monotonic module generation, not a pointer seed. */
    in->seed=(dc_u32)in->epoch^(dc_u32)(in->epoch>>32)*0x9e3779b9u;
    if(!dl_collect(&s->ladder,params,manager,group,in))return 0;
    return 1;
}

DR_EXPORT dc_u32 lab_director_sum_callback(void *controller,dc_u32 original,dc_u32 base_bits,
                                         dc_u32 formation_bits,dc_u32 catch_bits){
    dr_count(0);
    if(!lab_director_config.policy.enabled)return original;
    dc_u64 base=(dc_u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if((base&4095)||base>=(1ull<<39)-0x1400000){dr_count(1);return original;}
    dc_u64 mode=((dc_u64 (*)(void))(base+0x87c244))();
    if(!dr_ptr(mode)||dr_w(mode,4)==1||dr_w(mode,8)>1){dr_count(2);return original;}
    Snapshot a,z;dc_u32 ok=dr_collect((dc_u64)controller,base,&a);
    if(ok!=1){dr_count(ok==2?3:1);return original;}
    DirectorRuntimeRecord *r=&lab_director_records[a.policy.racer];dc_u32 expected=0;
    if(!__atomic_compare_exchange_n(&r->lock,&expected,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){dr_count(4);return original;}
    dc_u32 result=original;
    if(r->fault||(r->sequence&1)||r->sequence>~0ull-2||r->updates==~0ull){r->fault=1;goto done;}
    if(dr_collect((dc_u64)controller,base,&z)!=1||!dr_equal(&a,&z,sizeof(a))){dr_count(5);goto done;}
    FloatBits sum={.u=original},bb={.u=base_bits},ff={.u=formation_bits},cc={.u=catch_bits};
    a.policy.native_sum=sum.f;a.policy.native_base=bb.f;a.policy.native_formation=ff.f;a.policy.native_catch=cc.f;
    float promotion=0;
    lab_golden_promotion_read(&lab_golden_promotion_publication,
        a.policy.racer,a.policy.epoch,a.lifetime_generation,a.policy.owner,
        a.golden_role,a.golden_inventory,a.policy.tick,&promotion);
    a.policy.promotion_strength=promotion;
    DirectorCoreState next;dr_copy(&next,&r->state,sizeof(next));DirectorCoreOutput output;
    DirectorOpeningOutput opening_output;
    dc_u32 status=lab_director_opening_step(a.pressure.enabled?&a.pressure.policy:&lab_director_config.policy,&a.policy,&next,&output,
        &a.opening,&opening_output);
    if(status==DC_INVALID){dr_count(6);goto done;}
    DirectorLadderRecord ladder_next;
    dc_u32 ladder_ready=dl_prepare(&a.ladder,&a.policy,r->sequence,&ladder_next);
    DirectorPressureRecord pressure_next;
    dc_u32 pressure_ready=pr_prepare(&a.pressure,&a.policy,&opening_output,
        ladder_ready?&ladder_next.output:0,r->sequence,&pressure_next,&next,&output);
    if(ladder_ready)ladder_next.requested=pressure_ready&&pressure_next.request.valid&&
        opening_output.status!=DO_HOLD&&opening_output.status!=DO_FADE&&
        ladder_next.output.status!=DL_OFF;
    if(ladder_ready)ladder_next.blended=ladder_next.requested&&pressure_next.output.status==PP_READY;
    /* Recheck receipt after computation. Never commit private state from a
     * retired epoch; packet publication itself stays with the native caller. */
    Snapshot owner;dr_zero(&owner,sizeof(owner));
    owner.scene=a.scene;owner.system=a.system;owner.race=a.race;owner.group=a.group;
    if(!dr_lifetime(&owner)||owner.lifetime_token!=a.lifetime_token||
       owner.lifetime_generation!=a.lifetime_generation||owner.policy.epoch!=a.policy.epoch||
       dr_w(a.clock,0x38)!=6||dr_w(a.clock,0x44)!=a.policy.tick||
       !do_request_same(a.opening_sequence)||!pr_same(&a.pressure)||!dl_same(&a.ladder,a.group)){dr_count(5);goto done;}
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_ACQ_REL);
    r->updates++;r->controller=a.controller;r->scene=a.scene;r->system=a.system;r->race=a.race;
    r->group=a.group;r->manager=a.manager;r->parameters=a.parameters;
    dr_copy(&r->state,&next,sizeof(next));dr_copy(&r->input,&a.policy,sizeof(a.policy));dr_copy(&r->output,&output,sizeof(output));
    if(ladder_ready)dl_publish(&a.policy,&ladder_next,a.controller,a.parameters,a.clock,r->sequence+1);
    if(pressure_ready)pr_publish(&a.policy,&pressure_next,a.controller,a.clock,r->sequence+1);
    do_publish(&a.policy,&a.opening,&opening_output,a.controller,a.clock,a.opening_sequence,r->sequence+1);
    __atomic_add_fetch(&r->sequence,1,__ATOMIC_RELEASE);
    FloatBits wanted={.f=output.sum};result=wanted.u;dr_count(7);
done:
    __atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);return result;
}

DR_EXPORT const dc_u32 lab_director_lifetime_recheck_config[4]={1,88,1,0};
