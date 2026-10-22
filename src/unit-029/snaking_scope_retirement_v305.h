/* Passive continuation provenance, under the existing per-racer SnakeState
 * lock. Never follows saved pointers or changes native/frame/packet state. */
typedef struct {
    u64 sequence,events,dropped;
    u32 stage,racer,tick,retired_mask;
    u32 before_mask,after_mask,dt,physical_state;
    u32 phase,start,episodes; i32 direction;
    u64 driver,producer,clock,physics,drift,course;
    u32 contact_a,contact_b,role_flags,air;
    float residual;u32 reserved;
    SnakeHopFrame hop,ground;
    float chassis_up[3],motion[3],pending[3],up[3],target[3],position[3];
} SnakingScopeRecord;
_Static_assert(sizeof(SnakingScopeRecord)==392,"Snaking scope record ABI1");
EXPORT const u32 lab_snaking_scope_trace_config[6]={1,12,392,2,0,1};
/* Optional ABI1 scope extension: AD/AE/AF bytes and C4/A4/A5/D0 predicates.
 * Values belong to this capture; the earlier route-store cause remains separate. */
EXPORT const u32 lab_snaking_scope_selector_config[4]={1,7,140,0x0fffffffu};
_Static_assert(__builtin_offsetof(SnakingScopeRecord,reserved)==140,"Selector trace word");
EXPORT SnakingScopeRecord lab_snaking_scope_admission_records[12];
EXPORT SnakingScopeRecord lab_snaking_scope_retirement_records[12];

static u32 srt_mask(u32 racer){
    return (lab_snaking_hop_frames[racer].valid==1?1u:0u)|
           (lab_snaking_drift_frames[racer].valid==1?2u:0u);
}

EXPORT void lab_snaking_scope_trace(u32 racer,const SnakeState *s,void *physics,
    void *driver,u32 dt,u32 ds,u32 stage,u32 before){
    if(racer>=12||!ptr((u64)s)||!ptr((u64)physics)||!ptr((u64)driver)||
       s!=&lab_snaking_state[racer]||s->lock!=1||s->phase!=1||stage<1||stage>3||(before&~3u))return;
    u32 after=srt_mask(racer),retired=before&~after;
    if(stage!=1&&!retired)return;
    /* Driver/physics came from the dispatch's verified current owner graph.
     * These final checks make telemetry refuse mismatched test/call contexts. */
    u64 role=q(physics,0xf8),drift=q(physics,0x110);
    if(!ptr(role)||!ptr(drift)||q((void *)role,0x28)!=(u64)physics||
       q((void *)drift,8)!=role||w((void *)role,0xa8)!=racer||s->driver!=(u64)driver)return;
    SnakingScopeRecord *o=stage==1?&lab_snaking_scope_admission_records[racer]:
                                   &lab_snaking_scope_retirement_records[racer];
    u64 sequence=o->sequence;
    if(sequence&1||sequence>~0ull-2||o->events==~0ull){
        if(o->dropped!=~0ull)o->dropped++;
        return;
    }
    __atomic_store_n(&o->sequence,sequence+1,__ATOMIC_RELEASE);
    o->events++;o->stage=stage;o->racer=racer;o->tick=s->last;o->retired_mask=retired;
    o->before_mask=before;o->after_mask=after;o->dt=dt;o->physical_state=ds;
    o->phase=s->phase;o->start=s->start;o->episodes=s->episodes;o->direction=s->direction;
    o->driver=s->driver;o->producer=s->producer;o->clock=s->clock;o->physics=(u64)physics;
    o->drift=drift;o->course=q(driver,0x80);
    o->contact_a=b(physics,0x200);o->contact_b=b(physics,0x201);
    o->role_flags=w((void *)role,0x1cc);o->air=w(physics,0x26c);o->residual=f(physics,0x3c8);
    o->reserved=(u32)b(driver,0xad)|((u32)b(driver,0xae)<<8)|
        ((u32)b(driver,0xaf)<<16)|((u32)(b(driver,0xc4)!=0)<<24)|
        ((u32)(b(driver,0xa4)!=0)<<25)|((u32)(b(driver,0xa5)!=0)<<26)|
        ((u32)(b(driver,0xd0)!=0)<<27);
    o->hop=lab_snaking_hop_frames[racer];o->ground=lab_snaking_drift_frames[racer];
    for(u32 k=0;k<3;k++){
        o->chassis_up[k]=f(physics,0x14+4*k);o->motion[k]=f(physics,0x2fc+4*k);
        o->pending[k]=f(physics,0x308+4*k);o->up[k]=f(physics,0x320+4*k);
        o->target[k]=f(physics,0x32c+4*k);o->position[k]=f(physics,0x2c+4*k);
    }
    __atomic_store_n(&o->sequence,sequence+2,__ATOMIC_RELEASE);
}
