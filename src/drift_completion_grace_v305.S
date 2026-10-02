#include "register_context.inc"
.text
.balign 4
.global lab_drift_completion_hook
.type lab_drift_completion_hook,%function
lab_drift_completion_hook:
    save_context
    mov x0,x19
    mov w1,w23
    /* s0 is the native final direction dot product. */
    bl lab_drift_completion_hold
    cbz w0,.Lnative
    restore_context
.global lab_drift_completion_hold_return
lab_drift_completion_hold_return:
    b .
.Lnative:
    restore_context
    tbnz w23,#0,.Lrelease
.global lab_drift_completion_return
lab_drift_completion_return:
    b .
.Lrelease:
.global lab_drift_completion_release_return
lab_drift_completion_release_return:
    b .
.size lab_drift_completion_hook,.-lab_drift_completion_hook
