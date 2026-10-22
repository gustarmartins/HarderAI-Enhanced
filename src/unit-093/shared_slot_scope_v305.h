#ifndef HARDERAI_SHARED_SLOT_SCOPE_V305_H
#define HARDERAI_SHARED_SLOT_SCOPE_V305_H
#include "director_promotion_exchange_v305.h"
/* Experimental helper only. A qualified enclosing native callback must pin
 * these allocations for the ENTIRE submit/wake/join interval. Storage belongs
 * on that invocation's stack. No host writer or across-frame lease is allowed. */
typedef struct {
    dx_u64 wrapper,driver;
    dx_u32 racer,previous,assigned,reserved;
} SharedSlotCell;
typedef struct {
    dx_u32 active,count;
    dx_u64 epoch,generation,group,manager;
    SharedSlotCell cells[2];
    dx_u32 restored_mask,conflict_mask;
} SharedSlotLease;
_Static_assert(sizeof(SharedSlotLease)==112,"Shared slot scope ABI1");
dx_u32 lab_shared_slot_begin(dx_u64,void *,dx_u32,dx_u32,dx_u64,dx_u64,SharedSlotLease *);
dx_u32 lab_shared_slot_end(void *,dx_u64,dx_u64,SharedSlotLease *);
#endif
