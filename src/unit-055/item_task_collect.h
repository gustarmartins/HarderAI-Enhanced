#ifndef LAB_ITEM_TASK_COLLECT_H
#define LAB_ITEM_TASK_COLLECT_H
#include "item_source_policy.h"
typedef struct {item_u64 item,wrapper,context;} ItemTaskMember;
typedef struct {
    item_u64 base,manager,race,group,items,jobs;
    item_u32 count,threaded;
    ItemTaskMember members[12];
} ItemTaskRoster;
_Static_assert(sizeof(ItemTaskRoster)==344,"Item task roster ABI");
/* v3 only, at native item-task begin or inside its admitted item callback.
 * Caller must own current native task/batch and race construction lifetime.
 * This establishes roster/job identity, not an allocation or scheduling lease.
 * No game writes. Failure leaves output untouched. */
int lab_item_task_collect(item_u64,void *,void *,ItemTaskRoster *);
#endif
