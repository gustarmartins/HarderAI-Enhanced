/* OPTIONAL passive observer. Isolated fixture only; no production integration.
 * No native-object stores.
 * Native179BC4 already stored the event and the timer-update path completed.
 * Only the three qualified physical release callers produce observations. */
#include "native_turbo_commit_v305.h"
extern char lab_module_start[];
NTC_EXPORT const NativeTurboCommitConfig lab_native_turbo_commit_config={
    0x3130425255544148ull,1,NTC_SIZE,NTC_CAPACITY,0,0x179bc8,0x179bcc};
NTC_EXPORT NativeTurboCommit lab_native_turbo_commit_records[NTC_CAPACITY];

static ntc_u64 q(const void *p,ntc_u32 o){return *(const volatile ntc_u64 *)((const ntc_u8 *)p+o);}
static ntc_u32 w(const void *p,ntc_u32 o){return *(const volatile ntc_u32 *)((const ntc_u8 *)p+o);}
static ntc_u8 b(const void *p,ntc_u32 o){return *((const volatile ntc_u8 *)p+o);}
static ntc_u32 ptr(ntc_u64 p){return p>=0x10000&&!(p&7)&&p<(1ull<<39)-0x2000;}

NTC_EXPORT void lab_native_turbo_commit_publish(void *dash,ntc_u32 event,
                                                ntc_i32 duration,ntc_u64 caller){
    ntc_u64 base=(ntc_u64)lab_module_start-LAB_MAIN_TO_MODULE;
    if((base&4095)||base>=(1ull<<39)-0x1400000||!ptr((ntc_u64)dash))return;
    ntc_u32 tier=event==4?1:event==8?2:event==16?3:0;
    ntc_u64 expected=base+(tier==1?0x182334:tier==2?0x18237c:0x1822e8);
    if(!tier||caller!=expected)return;
    ntc_u32 kind=tier+2;
    ntc_u64 role=q(dash,8);if(!ptr(role))return;
    ntc_u64 physics=q((void *)role,0x28),context=q((void *)role,8);
    if(!ptr(physics)||!ptr(context)||q((void *)context,8)!=role||
       q((void *)physics,0xf8)!=role||q((void *)physics,0x118)!=(ntc_u64)dash||
       b((void *)role,0xe1)!=1||b((void *)role,0xe7))return;
    ntc_u32 racer=w((void *)role,0xa8);
    ntc_u64 drift=q((void *)physics,0x110);
    if(racer>=NTC_CAPACITY||w((void *)context,0x40)!=racer||!ptr(drift)||q((void *)drift,8)!=role)return;
    ntc_u64 root=q((void *)base,0x12f6388);if(!ptr(root))return;
    ntc_u64 single=q((void *)root,0);if(!ptr(single))return;
    ntc_u64 link=q((void *)single,0x20);if(!ptr(link))return;
    ntc_u64 scene=q((void *)link,8);if(!ptr(scene)||!(b((void *)scene,0x180)&4))return;
    ntc_u64 system=q((void *)scene,0x1b0);if(!ptr(system))return;
    ntc_u64 race=q((void *)system,0x238),clocks=q((void *)system,0x218);
    if(!ptr(race)||!ptr(clocks))return;
    ntc_u32 count=w((void *)race,0xc0);
    ntc_u64 contexts=q((void *)race,0xc8),clock=q((void *)clocks,0x50);
    if(count<=racer||count>NTC_CAPACITY||!ptr(contexts)||!ptr(clock)||
       q((void *)contexts,8*racer)!=context||w((void *)clock,0x38)!=6)return;
    ntc_u32 tick=w((void *)clock,0x44),dominant=w(dash,0x7c);
    ntc_i32 ticks_a=(ntc_i32)w(dash,0x1c+4*kind),ticks_b=(ntc_i32)w(dash,0x4c+4*kind);
    if(b(dash,0x10)!=1||w(dash,0x80)!=event||dominant<kind||dominant>=12||
       duration<0||ticks_a<duration||ticks_b<duration)return;

    NativeTurboCommit *s=&lab_native_turbo_commit_records[racer];ntc_u32 unlocked=0;
    if(!__atomic_compare_exchange_n(&s->lock,&unlocked,1,1,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
        ntc_u64 dropped=__atomic_load_n(&s->dropped,__ATOMIC_RELAXED);
        if(dropped!=~0ull)__atomic_compare_exchange_n(&s->dropped,&dropped,dropped+1,1,__ATOMIC_RELAXED,__ATOMIC_RELAXED);
        return;
    }
    if(s->fault)goto done;
    if((s->sequence&1)||s->sequence>~0ull-2||s->events==~0ull||s->by_tier[tier-1]==~0ull){
        s->fault=1;goto done;
    }
    /* Native caller owns the passed dash object; recheck the recorded chain
     * and clock. Host readers additionally require an independent race epoch. */
    if(q(dash,8)!=role||q((void *)role,0x28)!=physics||q((void *)physics,0xf8)!=role||
       q((void *)physics,0x118)!=(ntc_u64)dash||q((void *)physics,0x110)!=drift||
       q((void *)drift,8)!=role||q((void *)role,8)!=context||q((void *)context,8)!=role||
       q((void *)contexts,8*racer)!=context||q((void *)race,0xc8)!=contexts||
       q((void *)system,0x238)!=race||q((void *)system,0x218)!=clocks||
       q((void *)clocks,0x50)!=clock||q((void *)scene,0x1b0)!=system||
       q((void *)base,0x12f6388)!=root||q((void *)root,0)!=single||
       q((void *)single,0x20)!=link||q((void *)link,8)!=scene||!(b((void *)scene,0x180)&4)||
       w((void *)race,0xc0)!=count||w((void *)context,0x40)!=racer||
       w((void *)clock,0x38)!=6||w((void *)clock,0x44)!=tick||
       w((void *)role,0xa8)!=racer||b((void *)role,0xe1)!=1||b((void *)role,0xe7)||
       b(dash,0x10)!=1||w(dash,0x80)!=event||w(dash,0x7c)!=dominant||
       (ntc_i32)w(dash,0x1c+4*kind)!=ticks_a||(ntc_i32)w(dash,0x4c+4*kind)!=ticks_b)goto done;
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_ACQ_REL);
    s->events++;s->by_tier[tier-1]++;
    s->role=role;s->physics=physics;s->dash=(ntc_u64)dash;s->drift=drift;
    s->clock=clock;s->race_system=system;s->caller=caller;s->tick=tick;s->racer=racer;
    s->event=event;s->tier=tier;s->requested_kind=kind;s->dominant_kind=dominant;
    s->duration=duration;s->total_ticks=(ntc_i32)w(dash,0x14);
    s->kind_ticks_a=ticks_a;s->kind_ticks_b=ticks_b;
    s->role_flags=w((void *)role,0x1cc);s->role_bytes=w((void *)role,0xd0);
    s->active_remote=b((void *)role,0xe1)|((ntc_u32)b((void *)role,0xe7)<<8);
    s->star_ticks=(ntc_i32)w((void *)role,0x1e4);
    s->charge_bits=w((void *)drift,0x70);s->drift_state=w((void *)drift,0x10);s->reserved=0;
    __atomic_add_fetch(&s->sequence,1,__ATOMIC_RELEASE);
done:
    __atomic_store_n(&s->lock,0,__ATOMIC_RELEASE);
}
