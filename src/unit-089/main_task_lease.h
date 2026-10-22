#ifndef LAB_MAIN_TASK_LEASE_H
#define LAB_MAIN_TASK_LEASE_H
#include "native_admission.h"
typedef struct {u64 base,task_mutex,tree_mutex;} MainTaskLease;
u32 lab_main_task_try_lock(u64,void *,MainTaskLease *);
void lab_main_task_unlock(MainTaskLease *);
#endif
