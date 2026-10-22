/* Derived from retail ThreadMgr main-thread publication at 0x6258F0 and
 * SDK GetTlsValue at +0x906D0. No game writes or OS calls. Not installed. */
#include "queue_thread.h"
#include "queue_thread_layout.h"
#ifndef LAB_QUEUE_EXPECTED_PHASE
#define LAB_QUEUE_EXPECTED_PHASE 2
#endif
static int pointer(u64 p,u64 size) {
    return p>=0x10000 && !(p&7) && p<((1ull<<39)-size);
}
static u64 readq(u64 p) {return *(volatile u64 *)p;}
static u32 readw(u64 p) {return *(volatile u32 *)p;}
static u32 fail(QueueThreadSnapshot *s,u32 reason) {s->reason=reason;return 0;}

static u32 snapshot(u64 base,void *race,QueueThreadSnapshot *s,u32 require_race,u32 require_idle) {
    if (!s) return 0;
    *s=(QueueThreadSnapshot){0};
    if (!base || (base&4095) || base>=(1ull<<39)-0x1400000 ||
        (require_race && !pointer((u64)race,0x798))) return fail(s,1);
    __asm__ volatile("mrs %0, tpidrro_el0" : "=r"(s->tls));
    if (!pointer(s->tls,0x200)) return fail(s,2);
    s->nn_thread=readq(s->tls+0x1F8);
    if (!pointer(s->nn_thread,0x188)) return fail(s,3);
    /* Relocated global pointer must still designate the reviewed singleton. */
    if (readq(base+LAB_THREAD_MANAGER_GOT)!=base+LAB_THREAD_MANAGER_GLOBAL) return fail(s,4);
    s->thread_manager=readq(base+LAB_THREAD_MANAGER_GLOBAL);
    if (!pointer(s->thread_manager,0x90) ||
        readq(s->thread_manager)!=base+LAB_THREAD_MANAGER_VTABLE) return fail(s,5);
    s->slot=readw(s->thread_manager+0x88);
    if (s->slot>=32) return fail(s,6);
    s->main_thread=readq(s->thread_manager+0x80);
    if (!pointer(s->main_thread,0xE0) ||
        readq(s->main_thread)!=base+LAB_MAIN_THREAD_VTABLE) return fail(s,7);
    s->current_thread=readq(s->nn_thread+0x88+(u64)s->slot*8);
    if (s->current_thread!=s->main_thread) return fail(s,8);
    if (readq(s->main_thread+0xD8)!=s->nn_thread) return fail(s,9);
    if (readq(base+LAB_SCHEDULER_GOT)!=base+LAB_SCHEDULER_GLOBAL) return fail(s,10);
    s->scheduler=readq(base+LAB_SCHEDULER_GLOBAL);
    if (!pointer(s->scheduler,0xA0)) return fail(s,11);
    if (require_race) s->director_phase=readw((u64)race+0x790);
#ifndef LAB_QUEUE_ANY_PHASE
    if (require_race && s->director_phase!=LAB_QUEUE_EXPECTED_PHASE) return fail(s,12);
#endif
    s->pending_queues=readw(s->scheduler+0x98);
    if (require_idle && s->pending_queues) return fail(s,13);
    /* Repeat static ownership and mutable phase pointers around the read. */
    __asm__ volatile("dmb ishld" ::: "memory");
    if (readq(s->tls+0x1F8)!=s->nn_thread ||
        readq(base+LAB_THREAD_MANAGER_GOT)!=base+LAB_THREAD_MANAGER_GLOBAL ||
        readq(base+LAB_THREAD_MANAGER_GLOBAL)!=s->thread_manager ||
        readq(s->thread_manager)!=base+LAB_THREAD_MANAGER_VTABLE ||
        readw(s->thread_manager+0x88)!=s->slot ||
        readq(s->thread_manager+0x80)!=s->main_thread ||
        readq(s->main_thread)!=base+LAB_MAIN_THREAD_VTABLE ||
        readq(s->main_thread+0xD8)!=s->nn_thread ||
        readq(s->nn_thread+0x88+(u64)s->slot*8)!=s->main_thread ||
        readq(base+LAB_SCHEDULER_GOT)!=base+LAB_SCHEDULER_GLOBAL ||
        readq(base+LAB_SCHEDULER_GLOBAL)!=s->scheduler ||
        (require_idle && readw(s->scheduler+0x98)) || (require_race && readw((u64)race+0x790)!=
#ifdef LAB_QUEUE_ANY_PHASE
        s->director_phase
#else
        LAB_QUEUE_EXPECTED_PHASE
#endif
        )) return fail(s,14);
    return 1;
}
u32 lab_queue_thread_boundary(u64 base,void *race,QueueThreadSnapshot *s) {
    return snapshot(base,race,s,1,1);
}
u32 lab_queue_thread_idle(u64 base,QueueThreadSnapshot *s) {
    return snapshot(base,0,s,0,1);
}
u32 lab_queue_thread_current(u64 base,QueueThreadSnapshot *s) {
    return snapshot(base,0,s,0,0);
}
