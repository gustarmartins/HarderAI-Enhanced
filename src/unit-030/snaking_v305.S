#include "register_context.inc"
.text
.balign 4
.global lab_snaking_hook
.type lab_snaking_hook,%function
lab_snaking_hook:
    save_context
    mov x0,x19
    bl lab_snaking_dispatch
    restore_context
    ldp x29,x30,[sp,#0x10]
.global lab_snaking_return
lab_snaking_return:
    b .
.size lab_snaking_hook,.-lab_snaking_hook
