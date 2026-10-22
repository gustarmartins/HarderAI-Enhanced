/* Isolated v3.0.5 observer. Production integration requires the strong ready
 * binding below; golden_boundary_core fixtures qualify the compiled core. */
#ifndef LAB_GOLDEN_BOUNDARY_ADAPTER_V305_H
#define LAB_GOLDEN_BOUNDARY_ADAPTER_V305_H
#include "manual_intent.h"
enum { GOLDEN_ITEM_EXIT_RANK_V1=1, GOLDEN_CAPTURE_CAPACITY=8 };
typedef struct {
    u64 epoch,generation,driver,role,inventory,first_sequence,expiry_sequence;
    u32 racer,start_tick,end_tick,start_rank,end_rank,uses,rank_contract,rank_site;
    float start_progress,end_progress;
} GoldenBoundaryEvidence;
/* Required strong integration binding: verify exact retail/SDK dependencies,
 * all eight hooks/relocated returns, candidate lifetime ABI and default-off
 * feature configuration. No fallback implementation silently arms this file. */
u32 lab_golden_native_ready(u64 base);
void lab_golden_boundary_dispatch(u32 event,const u64 *registers,const void *native_sp);
void lab_golden_scalar_dispatch(u32 event,const u64 *registers,const void *native_sp,u32 previous_uses);
/* Existing COM/identity retirement dispatcher must call this before its route
 * becomes eligible for another generation. Main only, never clears borrowers. */
void lab_golden_boundary_retire(void);
/* Caller owns its CURRENT native director boundary and revalidated owner/clock.
 * Copy to private output once; never call using a cached native pointer receipt.
 * A successful copy is evidence, not a policy award or formation write. */
u32 lab_golden_boundary_take(u32 racer,u64 epoch,u64 generation,u64 driver,
                           u64 role,u64 inventory,u32 tick,GoldenBoundaryEvidence *out);
#endif
