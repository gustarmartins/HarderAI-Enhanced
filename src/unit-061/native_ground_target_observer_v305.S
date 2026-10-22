/* After native18AAA4 returns, before caller189020 copies the query normal. */
#include "register_context.inc"
.text
.balign 4
.global lab_native_ground_target_hook
.type lab_native_ground_target_hook,%function
lab_native_ground_target_hook:
    save_context
    mov w1,w0
    mov x0,x19
    add x2,sp,#0x3b0 /* original caller SP+90, above saved 0x320 context */
    add x3,sp,#0x3a0 /* original caller SP+80 */
    bl lab_native_ground_target_publish
    restore_context
    ldr w8,[sp,#0x90]
.global lab_native_ground_target_return
lab_native_ground_target_return:
    b .
.size lab_native_ground_target_hook,.-lab_native_ground_target_hook
