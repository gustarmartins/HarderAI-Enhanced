/* Private passive producer observer. No physics/input/parameter writes.
 * It records the actual native return and the query operands at caller189020.
 * ABI1 is diagnostic only. LAB_GROUND_TARGET_PROVENANCE adds the actual private
 * lifetime lease and exact current-state consumer used by the snaking probe. */
#include "native_ground_target_observer_v305.h"
extern char lab_module_start[];
NGT_EXPORT const ngt_u32 lab_native_ground_target_config[8]={
    NGT_ABI,NGT_SIZE,NGT_CAPACITY,0,0x18aaa4,0x189020,0x189024,0};
NGT_EXPORT NativeGroundTargetRecord lab_native_ground_target_records[NGT_CAPACITY];
static ngt_u64 ngt_q(const void *p,ngt_u32 o){return *(const volatile ngt_u64 *)((const ngt_u8 *)p+o);}
static ngt_u32 ngt_w(const void *p,ngt_u32 o){return *(const volatile ngt_u32 *)((const ngt_u8 *)p+o);}
static ngt_u8 ngt_b(const void *p,ngt_u32 o){return *((const volatile ngt_u8 *)p+o);}
static ngt_u32 ngt_ptr(ngt_u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x2000;}

static void ngt_publish_owned(void *physics,ngt_u32 accepted,
                 const ngt_u32 *normal,const ngt_u32 *hit,const NativeGroundLease *lease){
    ngt_u64 base=(ngt_u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if((base&4095)||base>=(1ull<<39)-0x1400000||!ngt_ptr((ngt_u64)physics)||
       !ngt_ptr((ngt_u64)normal)||!ngt_ptr((ngt_u64)hit)||accepted>1)return;
    ngt_u64 role=ngt_q(physics,0xf8);
    if(!ngt_ptr(role)||ngt_q((void *)role,0x28)!=(ngt_u64)physics||
       ngt_b((void *)role,0xe1)!=1||ngt_b((void *)role,0xe7))return;
    ngt_u64 context=ngt_q((void *)role,8);
    ngt_u32 racer=ngt_w((void *)role,0xa8);
    if(racer>=NGT_CAPACITY||!ngt_ptr(context)||ngt_q((void *)context,8)!=role||
       ngt_w((void *)context,0x40)!=racer)return;
    ngt_u64 root=ngt_q((void *)base,0x12f6388);if(!ngt_ptr(root))return;
    ngt_u64 single=ngt_q((void *)root,0);if(!ngt_ptr(single))return;
    ngt_u64 link=ngt_q((void *)single,0x20);if(!ngt_ptr(link))return;
    ngt_u64 scene=ngt_q((void *)link,8);if(!ngt_ptr(scene)||!(ngt_b((void *)scene,0x180)&4))return;
    ngt_u64 system=ngt_q((void *)scene,0x1b0);if(!ngt_ptr(system))return;
    ngt_u64 race=ngt_q((void *)system,0x238),clocks=ngt_q((void *)system,0x218);
    if(!ngt_ptr(race)||!ngt_ptr(clocks))return;
    ngt_u32 count=ngt_w((void *)race,0xc0);
    ngt_u64 contexts=ngt_q((void *)race,0xc8),clock=ngt_q((void *)clocks,0x50);
    if(count<=racer||count>NGT_CAPACITY||!ngt_ptr(contexts)||!ngt_ptr(clock)||
       ngt_q((void *)contexts,8*racer)!=context||ngt_w((void *)clock,0x38)!=6)return;
    NativeGroundScope scope;
    if(!lab_native_ground_scope_pin(lease,base,scene,system,race,&scope))return;
    ngt_u64 group=ngt_q((void *)race,0xb0);
    if(!ngt_ptr(group)||ngt_q((void *)group,0x28)!=race||(scope.group&&scope.group!=group))return;
    NativeGroundTargetPayload v;
    v.role=role;v.physics=(ngt_u64)physics;v.context=context;v.clock=clock;v.race_system=system;v.race=race;
    v.tick=ngt_w((void *)clock,0x44);v.racer=racer;v.accepted=accepted;
    v.role_flags=ngt_w((void *)role,0x1cc);v.role_bytes=ngt_w((void *)role,0xd0);
    v.active_remote=ngt_b((void *)role,0xe1)|((ngt_u32)ngt_b((void *)role,0xe7)<<8);
    v.air=ngt_w(physics,0x26c);v.contact=ngt_b(physics,0x200)|((ngt_u32)ngt_b(physics,0x201)<<8);
    v.mode=ngt_w(physics,0x230);v.selector=ngt_b(physics,0x27b);
    v.normal_flags=ngt_b(physics,0x20a)|((ngt_u32)ngt_b((void *)role,0xd8)<<8);
    v.displacement_bits=ngt_w(physics,0x568);v.reserved=0;
    for(ngt_u32 i=0;i<3;i++){
        v.old_road[i]=ngt_w(physics,0x17c+4*i);v.new_road[i]=ngt_w(normal,4*i);
        v.old_hit[i]=ngt_w(physics,0x188+4*i);v.new_hit[i]=ngt_w(hit,4*i);
        v.target[i]=ngt_w(physics,0x32c+4*i);v.actual[i]=ngt_w(physics,0x320+4*i);
        v.gravity[i]=ngt_w(physics,0x27c+4*i);v.motion[i]=ngt_w(physics,0x2fc+4*i);
        v.pending[i]=ngt_w(physics,0x308+4*i);v.chassis[i]=ngt_w(physics,0x14+4*i);
    }
    NativeGroundTargetRecord *s=&lab_native_ground_target_records[racer];ngt_u32 unlocked=0;
    if(!__atomic_compare_exchange_n(&s->lock,&unlocked,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        ngt_u64 dropped=__atomic_load_n(&s->dropped,__ATOMIC_RELAXED);
        if(dropped!=~0ull)__atomic_compare_exchange_n(&s->dropped,&dropped,dropped+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
        return;
    }
    if(s->fault)goto done;
    if((s->sequence&1)||s->sequence>~0ull-2||s->events==~0ull||
       s->accepted_count==~0ull||s->retained_count==~0ull){s->fault=1;goto done;}
    if(ngt_q(physics,0xf8)!=role||ngt_q((void *)role,0x28)!=(ngt_u64)physics||
       ngt_q((void *)role,8)!=context||ngt_q((void *)context,8)!=role||
       ngt_q((void *)contexts,8*racer)!=context||ngt_q((void *)race,0xc8)!=contexts||
       ngt_q((void *)system,0x238)!=race||ngt_q((void *)system,0x218)!=clocks||
       ngt_q((void *)clocks,0x50)!=clock||ngt_q((void *)scene,0x1b0)!=system||
       ngt_q((void *)base,0x12f6388)!=root||ngt_q((void *)root,0)!=single||
       ngt_q((void *)single,0x20)!=link||ngt_q((void *)link,8)!=scene||!(ngt_b((void *)scene,0x180)&4)||
       ngt_w((void *)race,0xc0)!=count||ngt_w((void *)context,0x40)!=racer||
       ngt_w((void *)clock,0x38)!=6||ngt_w((void *)clock,0x44)!=v.tick||
       ngt_w((void *)role,0xa8)!=racer||ngt_b((void *)role,0xe1)!=1||ngt_b((void *)role,0xe7))goto done;
    if(ngt_q((void *)race,0xb0)!=group||ngt_q((void *)group,0x28)!=race||
       !lab_native_ground_scope_same(lease,base,&scope))goto done;
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_ACQ_REL);
    s->value=v;s->events++;
    s->scope=scope;
    if(accepted)s->accepted_count++;else s->retained_count++;
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_RELEASE);
done:__atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);
}

NGT_EXPORT void lab_native_ground_target_publish(void *physics,ngt_u32 accepted,
                                      const ngt_u32 *normal,const ngt_u32 *hit){
    NativeGroundLease lease;
    if(!lab_native_ground_scope_enter(&lease))return;
    ngt_publish_owned(physics,accepted,normal,hit,&lease);
    lab_native_ground_scope_leave(&lease);
}

static ngt_u32 ngt_retained_owned(const void *physics,ngt_u64 clock,
                  ngt_u32 start,ngt_u32 now,const NativeGroundLease *lease){
    ngt_u64 base=(ngt_u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if((base&4095)||base>=(1ull<<39)-0x1400000||!ngt_ptr((ngt_u64)physics)||!ngt_ptr(clock)||start>=now)return 0;
    ngt_u64 role=ngt_q(physics,0xf8);
    if(!ngt_ptr(role)||ngt_q((void *)role,0x28)!=(ngt_u64)physics||ngt_b((void *)role,0xe1)!=1||ngt_b((void *)role,0xe7))return 0;
    ngt_u32 racer=ngt_w((void *)role,0xa8);if(racer>=NGT_CAPACITY)return 0;
    NativeGroundTargetRecord *r=&lab_native_ground_target_records[racer];ngt_u32 unlocked=0;
    if(!__atomic_compare_exchange_n(&r->lock,&unlocked,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
    ngt_u32 good=0;
    if(r->fault||!r->events||(r->sequence&1)||r->events>~0ull/2||
       r->sequence!=2*r->events||r->accepted_count>r->events||r->retained_count!=r->events-r->accepted_count)goto done;
    const NativeGroundTargetPayload *v=&r->value;const NativeGroundScope *scope=&r->scope;
    if(v->accepted||v->reserved||v->racer!=racer||v->role!=role||v->physics!=(ngt_u64)physics||
       v->clock!=clock||v->tick<start||v->tick>now||now-v->tick>1||v->air||v->contact!=0x101||
       v->active_remote!=1||(v->normal_flags&255)||
       !lab_native_ground_scope_same(lease,base,scope))goto done;
    /* After matching the current publication generation, follow only current
     * actor/race links. Historical query pointers are never dereferenced. */
    ngt_u64 context=ngt_q((void *)role,8);
    if(!ngt_ptr(context)||context!=v->context||ngt_q((void *)context,8)!=role||ngt_w((void *)context,0x40)!=racer||
       v->race_system!=scope->system||v->race!=scope->race||!ngt_ptr(scope->system)||!ngt_ptr(scope->race)||
       ngt_q((void *)scope->system,0x238)!=scope->race)goto done;
    ngt_u64 group=ngt_q((void *)scope->race,0xb0),clocks=ngt_q((void *)scope->system,0x218),contexts=ngt_q((void *)scope->race,0xc8);
    if(!ngt_ptr(group)||ngt_q((void *)group,0x28)!=scope->race||(scope->group&&scope->group!=group)||
       !ngt_ptr(clocks)||ngt_q((void *)clocks,0x50)!=clock||!ngt_ptr(contexts)||ngt_q((void *)contexts,8*racer)!=context||
       ngt_w((void *)scope->race,0xc0)<=racer||ngt_w((void *)scope->race,0xc0)>NGT_CAPACITY||
       ngt_w((void *)clock,0x38)!=6||ngt_w((void *)clock,0x44)!=now||ngt_w(physics,0x26c)||
       ngt_b(physics,0x200)!=1||ngt_b(physics,0x201)!=1||ngt_b(physics,0x20a)||
       ngt_w(physics,0x230)!=v->mode||ngt_b(physics,0x27b)!=v->selector||
       ((ngt_u32)ngt_b((void *)role,0xd8)<<8)!=v->normal_flags||
       ngt_w((void *)role,0x1cc)!=v->role_flags||ngt_w((void *)role,0xd0)!=v->role_bytes)goto done;
    for(ngt_u32 i=0;i<3;i++)if(ngt_w(physics,0x17c+4*i)!=v->new_road[i]||
        ngt_w(physics,0x32c+4*i)!=v->target[i]||ngt_w(physics,0x188+4*i)!=v->new_hit[i])goto done;
    if(ngt_q(physics,0xf8)!=role||ngt_q((void *)role,0x28)!=(ngt_u64)physics||
       ngt_q((void *)role,8)!=context||ngt_q((void *)context,8)!=role||ngt_q((void *)contexts,8*racer)!=context||
       ngt_w((void *)clock,0x38)!=6||ngt_w((void *)clock,0x44)!=now||
       !lab_native_ground_scope_same(lease,base,scope))goto done;
    good=1;
done:__atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);return good;
}
NGT_EXPORT ngt_u32 lab_native_ground_target_retained(const void *physics,ngt_u64 clock,ngt_u32 start,ngt_u32 now){
    NativeGroundLease lease;if(!lab_native_ground_scope_enter(&lease))return 0;
    ngt_u32 result=ngt_retained_owned(physics,clock,start,now,&lease);
    lab_native_ground_scope_leave(&lease);return result;
}
#include "native_accepted_ground_target_v305.h"
static ngt_u32 ngt_accepted_owned(const void *physics,ngt_u64 clock,
                  ngt_u32 start,ngt_u32 now,const NativeGroundLease *lease,NativeAcceptedGroundTarget *out){
    ngt_u64 base=(ngt_u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if((base&4095)||base>=(1ull<<39)-0x1400000||!ngt_ptr((ngt_u64)physics)||!ngt_ptr(clock)||start>=now)return 0;
    ngt_u64 role=ngt_q(physics,0xf8);
    if(!ngt_ptr(role)||ngt_q((void *)role,0x28)!=(ngt_u64)physics||ngt_b((void *)role,0xe1)!=1||ngt_b((void *)role,0xe7))return 0;
    ngt_u32 racer=ngt_w((void *)role,0xa8);if(racer>=NGT_CAPACITY)return 0;
    NativeGroundTargetRecord *r=&lab_native_ground_target_records[racer];ngt_u32 unlocked=0;
    if(!__atomic_compare_exchange_n(&r->lock,&unlocked,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return 0;
    ngt_u32 good=0;
    if(r->fault||!r->events||(r->sequence&1)||r->events>~0ull/2||
       r->sequence!=2*r->events||r->accepted_count>r->events||r->retained_count!=r->events-r->accepted_count)goto done;
    const NativeGroundTargetPayload *v=&r->value;const NativeGroundScope *scope=&r->scope;
    if(v->accepted!=1||v->reserved||v->racer!=racer||v->role!=role||v->physics!=(ngt_u64)physics||
       v->clock!=clock||v->tick<start||v->tick>now||now-v->tick>1||v->air||v->contact!=0x101||
       v->active_remote!=1||(v->normal_flags&255)||
       !lab_native_ground_scope_same(lease,base,scope))goto done;
    /* After matching the current publication generation, follow only current
     * actor/race links. Historical query pointers are never dereferenced. */
    ngt_u64 context=ngt_q((void *)role,8);
    if(!ngt_ptr(context)||context!=v->context||ngt_q((void *)context,8)!=role||ngt_w((void *)context,0x40)!=racer||
       v->race_system!=scope->system||v->race!=scope->race||!ngt_ptr(scope->system)||!ngt_ptr(scope->race)||
       ngt_q((void *)scope->system,0x238)!=scope->race)goto done;
    ngt_u64 group=ngt_q((void *)scope->race,0xb0),clocks=ngt_q((void *)scope->system,0x218),contexts=ngt_q((void *)scope->race,0xc8);
    if(!ngt_ptr(group)||ngt_q((void *)group,0x28)!=scope->race||(scope->group&&scope->group!=group)||
       !ngt_ptr(clocks)||ngt_q((void *)clocks,0x50)!=clock||!ngt_ptr(contexts)||ngt_q((void *)contexts,8*racer)!=context||
       ngt_w((void *)scope->race,0xc0)<=racer||ngt_w((void *)scope->race,0xc0)>NGT_CAPACITY||
       ngt_w((void *)clock,0x38)!=6||ngt_w((void *)clock,0x44)!=now||ngt_w(physics,0x26c)||
       ngt_b(physics,0x200)!=1||ngt_b(physics,0x201)!=1||ngt_b(physics,0x20a)||
       ngt_w(physics,0x230)!=v->mode||ngt_b(physics,0x27b)!=v->selector||
       ((ngt_u32)ngt_b((void *)role,0xd8)<<8)!=v->normal_flags||
       ngt_w((void *)role,0x1cc)!=v->role_flags||ngt_w((void *)role,0xd0)!=v->role_bytes)goto done;
    for(ngt_u32 i=0;i<3;i++)if(ngt_w(physics,0x17c+4*i)!=v->new_road[i]||
        ngt_w(physics,0x32c+4*i)!=v->target[i]||ngt_w(physics,0x188+4*i)!=v->new_hit[i])goto done;
    if(ngt_q(physics,0xf8)!=role||ngt_q((void *)role,0x28)!=(ngt_u64)physics||
       ngt_q((void *)role,8)!=context||ngt_q((void *)context,8)!=role||ngt_q((void *)contexts,8*racer)!=context||
       ngt_w((void *)clock,0x38)!=6||ngt_w((void *)clock,0x44)!=now||
       !lab_native_ground_scope_same(lease,base,scope))goto done;
    /* Recheck native operands after the final lifetime query. */
    for(ngt_u32 i=0;i<3;i++)if(ngt_w(physics,0x17c+4*i)!=v->new_road[i]||
        ngt_w(physics,0x32c+4*i)!=v->target[i]||ngt_w(physics,0x188+4*i)!=v->new_hit[i])goto done;
    if(ngt_w(physics,0x26c)||ngt_b(physics,0x200)!=1||ngt_b(physics,0x201)!=1||
       ngt_b(physics,0x20a)||ngt_w(physics,0x230)!=v->mode||ngt_b(physics,0x27b)!=v->selector||
       ngt_w((void *)role,0x1cc)!=v->role_flags||ngt_w((void *)role,0xd0)!=v->role_bytes||
       ((ngt_u32)ngt_b((void *)role,0xd8)<<8)!=v->normal_flags||
       ngt_b((void *)role,0xe1)!=1||ngt_b((void *)role,0xe7))goto done;
    NativeAcceptedGroundTarget value;
    for(ngt_u32 i=0;i<3;i++){value.actual[i]=v->actual[i];value.target[i]=v->target[i];}
    value.displacement_bits=v->displacement_bits;value.tick=v->tick;
    *out=value;good=1;
done:__atomic_store_n(&r->lock,0,__ATOMIC_RELEASE);return good;
}
NGT_EXPORT ngt_u32 lab_native_ground_target_accepted(const void *physics,ngt_u64 clock,
    ngt_u32 start,ngt_u32 now,NativeAcceptedGroundTarget *out){
    if(!out)return 0;
    NativeGroundLease lease;if(!lab_native_ground_scope_enter(&lease))return 0;
    ngt_u32 result=ngt_accepted_owned(physics,clock,start,now,&lease,out);
    lab_native_ground_scope_leave(&lease);return result;
}

