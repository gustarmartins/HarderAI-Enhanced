#ifndef LAB_GOLDEN_PROMOTION_POLICY_V305_H
#define LAB_GOLDEN_PROMOTION_POLICY_V305_H
/* Private pure scalar policy. Tokens below are opaque identities, never native
 * addresses to dereference. The caller owns one serialized state per race and
 * supplies a coherent full CPU roster at its qualified director boundary. */
#include "golden_boundary_adapter_v305.h"
enum { GP_ABI=1, GP_CAPACITY=12, GP_NO_RACER=12 };
enum { GP_IDLE=0, GP_PENDING=1, GP_ACTIVE=2, GP_RELEASING=3,
       GP_COOLDOWN=4, GP_RESET=5, GP_DISABLED=6, GP_INVALID=7 };
enum { GP_UNAVAILABLE=1 }; /* recovery, finished or otherwise unqualified */
enum { GP_FIRST=1, GP_SCOPE=2, GP_ROSTER=3, GP_CLOCK=4,
       GP_CONTINUITY=5, GP_CONFIG=6 };
typedef struct {
    u32 abi,enabled,max_gap_ticks,max_event_age_ticks;
    u32 min_rank_gain,sustain_ticks,attack_ticks,hold_ticks;
    u32 release_ticks,lost_grace_ticks,cooldown_ticks,reserved;
    float min_progress_gain,lost_distance;
} GoldenPromotionConfig;
typedef struct {
    u64 driver,role,inventory;
    u32 racer,rank,flags,reserved; /* actual zero-based race rank, not F.SLOT */
    float progress;
} GoldenPromotionCPU;
typedef struct {
    u64 epoch,generation,tune_revision;
    u32 tick,continuous,count,evidence_count;
    GoldenPromotionCPU cpus[GP_CAPACITY];
    /* At most one completed, immutable boundary evidence record per racer. */
    GoldenBoundaryEvidence evidence[GP_CAPACITY];
} GoldenPromotionFrame;
typedef struct {
    u64 seen_sequence;
    u32 seen_tick,pending,retained_ticks,start_rank,end_tick,reserved;
    float end_progress;
} GoldenPromotionRecord;
typedef struct {
    u64 epoch,generation,tune_revision;
    u32 initialized,last_tick,baseline_tick,roster_mask;
    GoldenPromotionConfig config;
    GoldenPromotionCPU owners[GP_CAPACITY]; /* indexed by racer, not row */
    GoldenPromotionRecord records[GP_CAPACITY];
    u32 winner,winner_rank,age,lost_ticks,releasing,release_age,cooldown,reserved;
    float initial_gap,strength,release_strength;
} GoldenPromotionState;
typedef struct {
    u64 driver,role,inventory;
    u32 status,racer,pending_count,reason;
    float strength;
} GoldenPromotionOutput;
/* Defaults are OFF. Durations use advancing race ticks, not host wall time. */
GoldenPromotionConfig lab_golden_promotion_default(void);
void lab_golden_promotion_clear(GoldenPromotionState *);
/* No allocation, I/O, native reads/writes, slot assignment or ratio addition.
 * Invalid frames retire the lease. Repeated qualified ticks freeze everything.
 * Returns out->status; null out refuses without changing caller state. */
u32 lab_golden_promotion_step(const GoldenPromotionConfig *,const GoldenPromotionFrame *,
                            GoldenPromotionState *,GoldenPromotionOutput *);
#endif
