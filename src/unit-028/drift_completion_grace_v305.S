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

.balign 4
.global lab_drift_completion_end_hook
.type lab_drift_completion_end_hook,%function
lab_drift_completion_end_hook:
    save_context
    mov x0,x19
    bl lab_drift_completion_end
    restore_context
    strb wzr,[x19,#0x41]
.global lab_drift_completion_end_return
lab_drift_completion_end_return:
    b .
.size lab_drift_completion_end_hook,.-lab_drift_completion_end_hook

/* Append to the existing completion assembly after register_context.inc. */
.text
.balign 4
.global lab_corner_arc_begin_hook
.type lab_corner_arc_begin_hook,%function
lab_corner_arc_begin_hook:
    save_context
    mov x0,x19
    bl lab_corner_arc_begin
    restore_context
    str wzr,[x19,#0x44]
.global lab_corner_arc_begin_return
lab_corner_arc_begin_return:
    b .
.size lab_corner_arc_begin_hook,.-lab_corner_arc_begin_hook
