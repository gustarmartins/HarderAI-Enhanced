#ifndef HARDERAI_SHARED_SLOT_RUNTIME_V305_H
#define HARDERAI_SHARED_SLOT_RUNTIME_V305_H
#include "shared_slot_jobs_v305.h"
typedef struct {u32 abi,enabled,first_tick,interval_ticks,duration_ticks,actors,reserved[2];} SharedSlotConfig;
typedef struct {
    u32 lock,fault;
    u64 sequence,updates,epoch,generation,group,clock;
    u32 last_tick,next_tick,end_tick,cycle,eligible_mask,selected_mask,applied_mask;
    u32 restored_mask,conflict_mask,status,visited_mask,applied_tick,queued_mask,reserved;
    u32 original_slots[12];
    u64 last_call;
} SharedSlotRecord;
_Static_assert(sizeof(SharedSlotConfig)==32,"Shared slot config ABI1");
_Static_assert(sizeof(SharedSlotRecord)==168,"Shared slot record ABI1");
#endif
