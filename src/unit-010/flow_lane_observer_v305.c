/* Isolated optional passive observer. No route, physics or game-state writes. */
#include "flow_lane_observer_v305.h"
extern char lab_module_start[];
FL_EXPORT const FlowLaneConfig lab_flow_lane_observer_config={
    0x31304e4c46494148ull,1,FL_RECORD_SIZE,FL_CAPACITY,2,0x7be418,0x7be44c,0x189118,0x18911c};
FL_EXPORT FlowLaneRecord lab_flow_lane_observer_records[2][FL_CAPACITY];
static fl_u64 q(const void *p,fl_u32 o){return *(const volatile fl_u64 *)((const fl_u8 *)p+o);}
static fl_u32 w(const void *p,fl_u32 o){return *(const volatile fl_u32 *)((const fl_u8 *)p+o);}
static fl_u8 b(const void *p,fl_u32 o){return *((const volatile fl_u8 *)p+o);}
static fl_u32 ptr(fl_u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x2000;}
static void words(fl_u32 *to,const void *from,fl_u32 off,fl_u32 n){for(fl_u32 i=0;i<n;i++)to[i]=w(from,off+4*i);}

static fl_u32 owners(void *physics,FlowLaneRecord *r){
    fl_u64 base=(fl_u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if((base&4095)||base>=(1ull<<39)-0x1400000||!ptr((fl_u64)physics))return 0;
    fl_u64 role=q(physics,0xf8);if(!ptr(role))return 0;
    fl_u64 context=q((void *)role,8);if(!ptr(context)||q((void *)role,0x28)!=(fl_u64)physics||
       q((void *)context,8)!=role||b((void *)role,0xe1)!=1||b((void *)role,0xe7))return 0;
    fl_u32 racer=w((void *)role,0xa8);
    fl_u64 driver=q((void *)context,0x18);if(racer>=12||!ptr(driver)||w((void *)context,0x40)!=racer)return 0;
    fl_u64 vehicle=q((void *)driver,0x88);
    if(!ptr(vehicle)||q((void *)vehicle,0x48)!=role||q((void *)vehicle,0x50)!=(fl_u64)physics)return 0;
    fl_u64 root=q((void *)base,0x12f6388);if(!ptr(root))return 0;
    fl_u64 single=q((void *)root,0);if(!ptr(single))return 0;
    fl_u64 link=q((void *)single,0x20);if(!ptr(link))return 0;
    fl_u64 scene=q((void *)link,8);if(!ptr(scene)||!(b((void *)scene,0x180)&4))return 0;
    fl_u64 system=q((void *)scene,0x1b0);if(!ptr(system))return 0;
    fl_u64 race=q((void *)system,0x238),clocks=q((void *)system,0x218);
    if(!ptr(race)||!ptr(clocks))return 0;
    fl_u32 count=w((void *)race,0xc0);
    fl_u64 contexts=q((void *)race,0xc8),clock=q((void *)clocks,0x50);
    if(count<=racer||count>12||!ptr(contexts)||!ptr(clock)||q((void *)contexts,8*racer)!=context||
       w((void *)clock,0x38)!=6)return 0;
    fl_u64 drift=q(physics,0x110);if(!ptr(drift)||q((void *)drift,8)!=role)return 0;
    r->role=role;r->physics=(fl_u64)physics;r->driver=driver;r->context=context;
    r->race=race;r->system=system;r->clock=clock;r->racer=racer;
    r->tick=w((void *)clock,0x44);r->stage=6;r->role_bytes=w((void *)role,0xd0);
    r->drift_state=w((void *)drift,0x10);r->speed_bits=w(physics,0x37c);
    words(r->position_bits,physics,0x2c,3);return 1;
}

static fl_u32 same_owner(const FlowLaneRecord *a,const FlowLaneRecord *b){
    return a->role==b->role&&a->physics==b->physics&&a->driver==b->driver&&
           a->context==b->context&&a->race==b->race&&a->system==b->system&&a->clock==b->clock;
}

static void publish(const FlowLaneRecord *r){
    FlowLaneRecord *s=&lab_flow_lane_observer_records[r->kind][r->racer];fl_u32 unlocked=0;
    if(!__atomic_compare_exchange_n(&s->lock,&unlocked,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        fl_u64 dropped=__atomic_load_n(&s->dropped,__ATOMIC_RELAXED);
        if(dropped!=~0ull)__atomic_compare_exchange_n(&s->dropped,&dropped,dropped+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
        return;
    }
    if(s->fault)goto done;
    fl_u32 new_owner=!s->events||!same_owner(s,r)||r->tick<s->tick||
                      (r->kind==0&&s->route!=r->route);
    if((s->sequence&1)||s->sequence>~0ull-2||s->events==~0ull||(new_owner&&s->generation==~0ull)){
        s->fault=1;goto done;
    }
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_ACQ_REL);
    s->events++;if(new_owner)s->generation++;
    /* Fixed-size payload; only this module-owned record is modified. */
    const fl_u64 *from=(const fl_u64 *)&r->role;fl_u64 *to=(fl_u64 *)&s->role;
    for(fl_u32 i=0;i<(FL_RECORD_SIZE-40)/8;i++)to[i]=from[i];
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_RELEASE);
done:
    __atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);
}

FL_EXPORT void lab_flow_lane_target_publish(void *route,fl_u32 x,fl_u32 y,fl_u32 z){
    if(!ptr((fl_u64)route))return;
    fl_u64 driver=q(route,0x40);if(!ptr(driver))return;
    fl_u64 vehicle=q((void *)driver,0x88);if(!ptr(vehicle))return;
    fl_u64 physics=q((void *)vehicle,0x50);
    FlowLaneRecord r={0},after={0};
    if(!owners((void *)physics,&r)||r.driver!=driver)return;
    fl_u64 state=q((void *)driver,0x50);if(!ptr(state)||q((void *)state,0x88)!=driver)return;
    fl_u64 nav=q((void *)state,0xb0);if(!ptr(nav)||q((void *)nav,0x58)!=driver)return;
    fl_u64 producer=q((void *)nav,0x70);
    if(!ptr(producer)||q((void *)producer,0x70)!=driver||q((void *)producer,0x88)!=(fl_u64)route)return;
    fl_u64 current=q(route,0x58),following=q(route,0x50);
    if(!ptr(current)||!ptr(following))return;
    r.kind=0;r.route=(fl_u64)route;r.current=current;r.following=following;r.mode=-1;
    r.current_key=w((void *)current,0x8c);r.following_key=w((void *)following,0x8c);
    r.center_bits[0]=x;r.center_bits[1]=y;r.center_bits[2]=z;
    words(r.side_bits,route,0x8c,3);r.offset_bits=w(route,0x98);words(r.target_bits,route,0x9c,3);
    if(!owners((void *)physics,&after)||!same_owner(&r,&after)||r.tick!=after.tick||
       r.role_bytes!=after.role_bytes||q(route,0x40)!=driver||q((void *)driver,0x50)!=state||
       q((void *)state,0xb0)!=nav||q((void *)nav,0x70)!=producer||
       q((void *)producer,0x88)!=(fl_u64)route||q(route,0x58)!=current||q(route,0x50)!=following||
       w((void *)current,0x8c)!=r.current_key||w((void *)following,0x8c)!=r.following_key)return;
    publish(&r);
}

FL_EXPORT void lab_flow_lane_contact_publish(void *physics,const void *response,fl_u32 admitted,const void *native_sp){
    if(admitted>1||!ptr((fl_u64)response)||!ptr((fl_u64)native_sp))return;
    FlowLaneRecord r={0},after={0};
    if(!owners(physics,&r))return;
    r.kind=1;r.admitted=admitted;r.response=(fl_u64)response;r.native_sp=(fl_u64)native_sp;
    r.response_flags=w(response,0);r.mode=-1;
    if(admitted){
        if(!(r.response_flags&0x2000))return;
        words(r.flow_bits,native_sp,0x70,3);r.mode=(fl_i32)w(native_sp,0x50);
    }
    if(!owners(physics,&after)||!same_owner(&r,&after)||r.tick!=after.tick||
       r.role_bytes!=after.role_bytes||w(response,0)!=r.response_flags)return;
    publish(&r);
}
