/* Private prototype. All three hooks are one indivisible feature. Bind native
 * symbols to the exact 3.0.5 image and verify complete callers/callees, including
 * native frame sizes. No installation or pipeline integration implied here. */
.text
.p2align 2
#include "register_context.inc"
.global lab_racer_follow_update
.type lab_racer_follow_update,%function
lab_racer_follow_update:
    /* Called by replacement BL at4CE2C; 64-byte stack record. X28 is
     * native ABI callee-saved. State4 saves/restores it before secondary48B14. */
    sub sp,sp,#0x40
    stp x28,x30,[sp,#0x30]
    mov x28,sp
    stp x0,x1,[sp,#8]
    str x2,[sp,#0x18]
    str xzr,[sp,#0x20]
    str xzr,[sp,#0x28]
    mov w9,#0x4a46
    movk w9,#0x3247,lsl #16
    str x9,[sp]
    bl lab_follow_native_update
    ldp x28,x30,[sp,#0x30]
    add sp,sp,#0x40
    ret
.size lab_racer_follow_update,.-lab_racer_follow_update

.global lab_racer_follow_target
.type lab_racer_follow_target,%function
lab_racer_follow_target:
    /* Replacement B at4C078. Native4894C/48A38/4BEC8 frames total0x100. */
    save_context
    add x9,sp,#0x420
    cmp x28,x9
    b.ne .Lfollow_original
    mov x4,x2
    mov x3,x1
    mov x2,x0
    mov x1,x19
    mov x0,x28
    bl lab_racer_follow_deny
    cbz w0,.Lfollow_original
    restore_context
    b lab_follow_native_abort
.Lfollow_original:
    restore_context
    bl lab_follow_native_writer
    b lab_follow_native_after_target
.size lab_racer_follow_target,.-lab_racer_follow_target

.global lab_racer_follow_secondary
.type lab_racer_follow_secondary,%function
lab_racer_follow_secondary:
    /* Replacement B at48CF0; native4894C/48B14 frames total0x60. Preserve all
     * preceding48B14 RNG, timer and side-state execution even on refusal. */
    save_context
    add x9,sp,#0x380
    cmp x28,x9
    b.ne .Lsecondary_original
    mov x4,x2
    mov x3,x1
    mov x2,x0
    mov x1,x19
    mov x0,x28
    bl lab_racer_follow_secondary_deny
    cbz w0,.Lsecondary_original
    restore_context
    b lab_follow_native_after_secondary
.Lsecondary_original:
    restore_context
    bl lab_follow_native_writer
    b lab_follow_native_after_secondary
.size lab_racer_follow_secondary,.-lab_racer_follow_secondary
