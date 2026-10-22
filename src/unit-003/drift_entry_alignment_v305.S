#include "register_context.inc"
.text
.balign 4
.global lab_drift_entry_alignment_hook
.type lab_drift_entry_alignment_hook,%function
lab_drift_entry_alignment_hook:
    /* Preserve the native FCMP/CCMP refusal before reading any actor fields.
     * The existing physical-ownership hook at5CA74 stays in the native path. */
    b.eq .Lentry_native_refused
    save_context
    mov x0,x19
    bl lab_drift_entry_alignment_defer
    cbz w0,.Lentry_native
    restore_context
.global lab_drift_entry_alignment_defer_return
lab_drift_entry_alignment_defer_return:
    b .
.Lentry_native:
    restore_context
.global lab_drift_entry_alignment_native_return
lab_drift_entry_alignment_native_return:
    b .
.Lentry_native_refused:
.global lab_drift_entry_alignment_upstream_return
lab_drift_entry_alignment_upstream_return:
    b .
.size lab_drift_entry_alignment_hook,.-lab_drift_entry_alignment_hook
