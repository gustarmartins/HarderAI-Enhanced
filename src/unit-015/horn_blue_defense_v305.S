#include "register_context.inc"
.text
.balign 4
.global lab_horn_blue_hook
.type lab_horn_blue_hook,%function
lab_horn_blue_hook:
    save_context
    mov x0,x19
    adrp x1,lab_module_start
    add x1,x1,:lo12:lab_module_start
    movz x9,#(LAB_MAIN_TO_MODULE & 0xffff)
    movk x9,#((LAB_MAIN_TO_MODULE >> 16) & 0xffff),lsl #16
    sub x1,x1,x9
    bl lab_horn_blue_current
    cmp w0,#1
    b.eq .Lhorn_keep
    cmp w0,#2
    b.eq .Lhorn_use
    restore_context
    str wzr,[x19,#0xc0]
.global lab_horn_blue_return
lab_horn_blue_return:
    b .
.Lhorn_keep:
    restore_context
    strb wzr,[x19,#0xb8]
    str wzr,[x19,#0xc0]
    b lab_horn_blue_consume
.Lhorn_use:
    restore_context
    /* Keep register context intact while setting the native pending-use latch. */
    str wzr,[x19,#0xc0]
    str x9,[sp,#-16]!
    mov w9,#1
    strb w9,[x19,#0xb8]
    ldr x9,[sp],#16
.global lab_horn_blue_consume
lab_horn_blue_consume:
    b .
.size lab_horn_blue_hook,.-lab_horn_blue_hook
