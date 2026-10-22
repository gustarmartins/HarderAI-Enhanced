/* Distinct phase2 readers. No new native mutex is held over worker execution. */
#define lab_queue_thread_boundary lab_shared_jobs_queue_boundary
#define lab_queue_thread_idle lab_shared_jobs_queue_idle
#define lab_queue_thread_current lab_shared_jobs_queue_current
#define lab_main_task_prerequisite lab_shared_jobs_task_prerequisite
#define lab_main_generation lab_shared_jobs_generation
#include "queue_thread.c"
#include "main_task_prerequisite.c"
