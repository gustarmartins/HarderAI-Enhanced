/* OPTIONAL hook qualified in an isolated fixture; no production installation. */
#include "register_context.inc"
.text
.balign 4
.global lab_native_turbo_commit_hook
.type lab_native_turbo_commit_hook,%function
lab_native_turbo_commit_hook:
    save_context
    mov x0,x20
    mov w1,w19
    mov w2,w21
    /*179534's native prologue preserves its original caller LR at FP+8. */
    ldr x3,[x29,#8]
    bl lab_native_turbo_commit_publish
    restore_context
    /* Displaced native179BC8, exactly once: return the native duration. */
    mov w0,w21
.global lab_native_turbo_commit_return
lab_native_turbo_commit_return:
    b .
.size lab_native_turbo_commit_hook,.-lab_native_turbo_commit_hook
