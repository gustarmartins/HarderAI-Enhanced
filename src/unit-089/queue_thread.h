#ifndef LAB_QUEUE_THREAD_H
#define LAB_QUEUE_THREAD_H
#include "native_admission.h"
/* Main-thread/scheduler prerequisite only: this is NOT an allocation lease.
 * Construction generation and complete caller ownership remain separate gates.
 * Caller must validate the retail functions/layout and module before using it. */
typedef struct {
    u64 tls,nn_thread,thread_manager,main_thread,current_thread,scheduler;
    u32 slot,director_phase,pending_queues,reason;
} QueueThreadSnapshot;
_Static_assert(sizeof(QueueThreadSnapshot)==64,"Queue thread diagnostic size");
u32 lab_queue_thread_boundary(u64 base,void *race,QueueThreadSnapshot *out);
/* Canonical main thread and idle scheduler only; no director is accessed.
 * Suitable as one prerequisite inside the real frame-transition scope after
 * old race destruction. Does not create an object or native resource lease. */
u32 lab_queue_thread_idle(u64 base,QueueThreadSnapshot *out);
/* Canonical MainThread identity only; queued work is observed, not excluded.
 * Requires a separate native ownership boundary. Never substitutes for idle
 * transition/admission checks or grants access to a race by itself. */
u32 lab_queue_thread_current(u64 base,QueueThreadSnapshot *out);
#endif
