#ifndef LAB_ITEM_SOURCE_POLICY_H
#define LAB_ITEM_SOURCE_POLICY_H
typedef unsigned int item_u32;
typedef unsigned long long item_u64;
/* Coherent native-thread observations, never host-supplied dereference targets.
 * The adapter must prove construction generation and object ownership first. */
typedef struct {
    item_u64 generation, wrapper, item, local;
    item_u32 racer, native_cpu, latch, local_pressed, ai_pressed, slot;
} ItemSourceObservation;
typedef struct {
    item_u64 generation, wrapper, item, local;
    item_u32 racer, sequence, source, reason;
    item_u32 slot, reserved;
} ItemSourcePlan;
enum { ITEM_SOURCE_SKIP=-1, ITEM_SOURCE_LOCAL=0, ITEM_SOURCE_AI=1 };
enum { ITEM_PLAN_NATIVE=0, ITEM_PLAN_PHYSICAL=1, ITEM_PLAN_NOT_NEUTRAL=2,
       ITEM_PLAN_NO_LOCAL=3, ITEM_PLAN_LOCAL_LOST=4 };
_Static_assert(sizeof(ItemSourceObservation)==56,"Item observation ABI");
_Static_assert(sizeof(ItemSourcePlan)==56,"Item plan ABI");
/* Zero-initialized plan; generation is monotonically increasing within one
 * module lifetime. Call at a proved race boundary, not on a host request change.
 * An accepted choice (including native fallback) is immutable for that race.
 * Request: even nonzero sequence, physical 0/1. Returns 0 on invalid input. */
int lab_item_source_prepare(ItemSourcePlan *,const ItemSourceObservation *,item_u32,item_u32);
/* SKIP means retain latch and bypass this item update, NOT switch to AI.
 * Losing/replacing the selected local source suspends this plan until the next
 * race, even if the old address returns. No automatic reconnect release edge.
 * A future native trampoline must branch to the verified early-return path. */
int lab_item_source_decide(ItemSourcePlan *,const ItemSourceObservation *);
/* Native item-task entry only. The caller owns the proved construction token
 * and object lifetime. Failure leaves output untouched and is not permission
 * to fall back from an accepted physical plan to AI input. */
int lab_item_source_collect(item_u64,item_u64,void *,void *,item_u32,ItemSourceObservation *);
#endif
