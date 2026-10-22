#include "register_context.inc"
.text
.balign 4
.global lab_drift_pace_hook
.type lab_drift_pace_hook,%function
lab_drift_pace_hook:
    save_all
    mov x0,x19
    mov x1,x20
    bl lab_drift_pace_apply
    restore_all
    /* Native 181900, after either old or scoped/adaptive pace returns. */
    add x9,x19,#0x1f4
.global lab_drift_pace_return
lab_drift_pace_return:
    b .
.size lab_drift_pace_hook,.-lab_drift_pace_hook
