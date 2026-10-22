#include "register_context.inc"
.text
.balign 4
.global lab_star_defense_hook
.type lab_star_defense_hook,%function
lab_star_defense_hook:
    save_context
    mov x0,x19
    adrp x1,lab_module_start
    add x1,x1,:lo12:lab_module_start
    movz x9,#(LAB_MAIN_TO_MODULE & 0xffff)
    movk x9,#((LAB_MAIN_TO_MODULE >> 16) & 0xffff),lsl #16
    sub x1,x1,x9
    bl lab_star_defense_current
    cbnz w0,.Lstar_use
    restore_context
    ldrb w8,[x19,#0xf4]
.global lab_star_defense_return
lab_star_defense_return:
    b .
.Lstar_use:
    restore_context
    /* Retail62230 sets its pending-use latch and retains stock consumption. */
.global lab_star_defense_use
lab_star_defense_use:
    b .
.size lab_star_defense_hook,.-lab_star_defense_hook
