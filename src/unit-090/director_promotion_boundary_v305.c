/* Separate phase-5 witnesses. Existing phase-2 consumers remain unchanged. */
#define LAB_QUEUE_EXPECTED_PHASE 5
#define lab_queue_thread_boundary lab_promotion_queue_boundary
#define lab_queue_thread_idle lab_promotion_queue_idle
#define lab_queue_thread_current lab_promotion_queue_current
#define lab_main_task_prerequisite lab_promotion_task_prerequisite
#define lab_main_task_try_lock lab_promotion_task_try_lock
#define lab_main_task_unlock lab_promotion_task_unlock
#define lab_main_generation lab_promotion_generation
#include "queue_thread.c"
#include "main_task_prerequisite.c"
#include "main_task_lease.c"
