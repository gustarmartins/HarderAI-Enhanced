#include "register_context.inc"
.text
.balign 4
.global lab_early_drift_pid_entry_hook
.type lab_early_drift_pid_entry_hook,%function
lab_early_drift_pid_entry_hook:
    /* Displaced B.HI uses CMP counter,#29 flags from retail5CEB8. Preserve
     * the ordinary counter>=30 path without even calling the new predicate. */
    b.hi .Learly_pid_native_late
    save_context
    mov x0,x19
    bl lab_early_drift_pid_admit
    cbnz w0,.Learly_pid_admitted
    restore_context
.global lab_early_drift_pid_entry_return
lab_early_drift_pid_entry_return:
    b .
.Learly_pid_admitted:
    restore_context
.Learly_pid_native_late:
.global lab_early_drift_pid_native_late
lab_early_drift_pid_native_late:
    b .
.size lab_early_drift_pid_entry_hook,.-lab_early_drift_pid_entry_hook

.balign 4
.global lab_early_drift_pid_finish_hook
.type lab_early_drift_pid_finish_hook,%function
lab_early_drift_pid_finish_hook:
    /* Only the newly admitted early route can reach5D160 with counter<30.
     * PID has already stored its history and angle. Keep the original early
     * maneuver lifetime: do not run later route-completion exits prematurely.
     * No second admission/readback can accidentally expose those late exits. */
    save_context
    ldr w0,[x19,#0x44]
    cmp w0,#30
    b.lo .Learly_pid_finish
    restore_context
    ldp s12,s0,[sp,#0x10]
.global lab_early_drift_pid_finish_return
lab_early_drift_pid_finish_return:
    b .
.Learly_pid_finish:
    restore_context
.global lab_early_drift_pid_early_finish
lab_early_drift_pid_early_finish:
    b .
.size lab_early_drift_pid_finish_hook,.-lab_early_drift_pid_finish_hook
