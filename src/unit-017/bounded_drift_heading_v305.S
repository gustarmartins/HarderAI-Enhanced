#include "register_context.inc"
.text
.balign 4
.global lab_bounded_drift_heading_hook
.type lab_bounded_drift_heading_hook,%function
lab_bounded_drift_heading_hook:
    /* Native6E96C: X19 owns the movement state. Existing flags/axis were
     * published; native6E970 will load our narrowly owned packet10 value. */
    save_context
    mov x0,x19
    bl lab_bounded_drift_heading_dispatch
    restore_context
    ldr x1,[x19,#0xc0]
.global lab_bounded_drift_heading_return
lab_bounded_drift_heading_return:
    b .
.size lab_bounded_drift_heading_hook,.-lab_bounded_drift_heading_hook
