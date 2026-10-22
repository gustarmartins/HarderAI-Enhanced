#ifndef HARDERAI_DIRECTOR_CORE_V305_H
#define HARDERAI_DIRECTOR_CORE_V305_H
/* Private pure policy ABI. No native addresses, allocation, I/O or game writes.
 * Tokens are opaque owner/generation identities supplied by a qualified caller.
 * The caller serializes each actor state; records from other actors are unused. */
typedef unsigned int dc_u32;
typedef unsigned long long dc_u64;
#define DC_CAPACITY 12u
#define DC_ABI 1u
enum { DC_BATTLE=1,DC_CHASE=2,DC_SPREAD=3 };
enum { DC_LEAD=1,DC_ATTACK=2,DC_FIELD=3,DC_RECOVERY=4,DC_EARNED=5 };
enum { DC_BOOST=1,DC_OFFROAD=2,DC_UNAVAILABLE=4 };
enum { DC_PASSTHROUGH=0,DC_APPLIED=1,DC_INVALID=2,DC_RESET=3,DC_BOOST_HELD=4 };
enum { DC_RESET_NONE=0,DC_RESET_FIRST=1,DC_RESET_OWNER=2,DC_RESET_TUNE=3,
       DC_RESET_CLOCK=4,DC_RESET_ROSTER=5,DC_RESET_DISABLED=6 };
typedef struct {
    dc_u32 abi,enabled,max_gap_ticks,reserved;
    float catch_authority,formation_authority,spacing,response_ticks;
    float phase_variation,depth_amplitude;
} DirectorCoreConfig;
typedef struct {
    dc_u64 owner;
    float progress;
    dc_u32 racer,slot,reserved;
} DirectorCorePeer;
typedef struct {
    dc_u64 epoch,owner,tune_revision;
    dc_u32 tick,seed,count,racer;
    dc_u32 flags,human_valid,reserved[2];
    float human_progress,human_weight;
    float native_sum,native_base,native_catch,native_formation,native_rung;
    float up,down,cu_min,cu_max,cu_ratio,cd_min,cd_max,cd_ratio;
    float promotion_strength;
    float targets[DC_CAPACITY];
    DirectorCorePeer peers[DC_CAPACITY];
} DirectorCoreInput;
typedef struct {
    dc_u64 epoch,owner,tune_revision,roster_signature;
    dc_u32 initialized,last_tick,seed,phase_key,cohort,reserved;
    float catch_term,formation_term,band_low,band_high;
} DirectorCoreState;
typedef struct {
    dc_u32 status,phase,cohort,reset_reason;
    float sum,catch_term,formation_term,band_low,band_high;
    float gap,target,rung,human_term,leader_term,local_term;
} DirectorCoreOutput;
/* Config defaults: enabled, full authored authority, 240-unit spacing,
 * 54-update response, 120-update discontinuity limit, seeded phase variation. */
DirectorCoreConfig lab_director_core_default(void);
/* Returns output.status; valid out required. Invalid input does not mutate
 * private state. Disabled/unavailable retires its state. A reset establishes
 * the current native baseline without a retrospective catch-up impulse. */
dc_u32 lab_director_core_step(const DirectorCoreConfig *,const DirectorCoreInput *,
                             DirectorCoreState *,DirectorCoreOutput *);
#endif
