#ifndef HARDERAI_BOUNDED_DRIFT_HEADING_OBSERVATION_V305_H
#define HARDERAI_BOUNDED_DRIFT_HEADING_OBSERVATION_V305_H
/* Private latest successful observation after an actual changed packet10 store.
 * CAS contention skips capture. Counts are lower bounds, never turbo counts.
 * Even sequence publishes; fault is sticky and stops updates without wrap.
 */
typedef struct {
    u32 sequence, fault;
    u64 observed_stores;
    u64 driver,producer,physics,clock,state,packet;
    u32 tick,racer,scope,drift_state;
    float original,result,error,correction,clearance,speed;
    u64 reserved[3];
} BoundedHeadingObservation;
_Static_assert(sizeof(BoundedHeadingObservation)==128,"Heading observation stride");
_Static_assert(__builtin_offsetof(BoundedHeadingObservation,driver)==16,"Heading owner ABI");
_Static_assert(__builtin_offsetof(BoundedHeadingObservation,tick)==64,"Heading time ABI");
_Static_assert(__builtin_offsetof(BoundedHeadingObservation,original)==80,"Heading value ABI");
EXPORT const u32 lab_bounded_drift_heading_observation_config[4]={1,128,12,1};
EXPORT BoundedHeadingObservation lab_bounded_drift_heading_observations[12];

static void observe_heading(u32 racer,u64 driver,u64 producer,u64 physics,u64 clock,
                            u64 state,u64 packet,u32 tick,u32 scope,u32 drift_state,
                            float original,float result,float error,float correction,
                            float clearance,float speed){
    if(racer>=12)return;
    BoundedHeadingObservation *r=lab_bounded_drift_heading_observations+racer;
    u32 sequence=__atomic_load_n(&r->sequence,__ATOMIC_ACQUIRE);
    if(sequence&1u||sequence>=0xfffffffeu)return;
    if(!__atomic_compare_exchange_n(&r->sequence,&sequence,sequence+1u,1,
                                    __ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;
    if(r->fault){__atomic_store_n(&r->sequence,sequence,__ATOMIC_RELEASE);return;}
    if(sequence>=0xfffffffcu||r->observed_stores==~0ull){
        r->fault=(sequence>=0xfffffffcu?2u:0u)|(r->observed_stores==~0ull?1u:0u);
        __atomic_store_n(&r->sequence,sequence+2u,__ATOMIC_RELEASE);return;
    }
    r->observed_stores++;
    r->driver=driver;r->producer=producer;r->physics=physics;r->clock=clock;
    r->state=state;r->packet=packet;r->tick=tick;r->racer=racer;
    r->scope=scope;r->drift_state=drift_state;
    r->original=original;r->result=result;r->error=error;r->correction=correction;
    r->clearance=clearance;r->speed=speed;
    __atomic_store_n(&r->sequence,sequence+2u,__ATOMIC_RELEASE);
}
#endif
