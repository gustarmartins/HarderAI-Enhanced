#ifndef HARDERAI_SHARED_SLOT_JOBS_V305_H
#define HARDERAI_SHARED_SLOT_JOBS_V305_H
#define LAB_SHARED_JOB_FRAME_BYTES 0x180
#ifndef __ASSEMBLER__
#include "shared_slot_scope_v305.h"
#include "manual_intent.h"
/* Lives ABOVE the native submit/wake/join stack frames, until their joined
 * return. A C-local receipt or storage below a restored SP is not sufficient.
 * The enclosing native calc traversal, not this receipt, owns the tree pin. */
typedef struct {
    u64 magic,native_sp,thread,group,epoch,generation;
    SharedSlotLease slots;
    u64 reserved_runtime_leases[7];
    u32 status,kind,owned,reserved;
    u64 scratch[19];
} SharedSlotJobFrame;
_Static_assert(sizeof(SharedSlotJobFrame)==LAB_SHARED_JOB_FRAME_BYTES,"Shared jobs frame ABI1");
void lab_shared_jobs_prepare(const void *,SharedSlotJobFrame *,u64);
void lab_shared_jobs_finish(const void *,SharedSlotJobFrame *,u64);
#endif
#endif
