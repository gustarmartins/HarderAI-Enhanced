/* Preserve native FADD, architectural context and FPSR. Replace only S0. */
#include "register_context.inc"
.text
.balign 4
.global lab_director_sum_hook
.type lab_director_sum_hook,%function
lab_director_sum_hook:
    fadd s0,s0,s1
    save_context
    mov x0,x19
    ldr w1,[sp,#0x100]
    ldr w2,[sp,#0x1b0]
    ldr w3,[sp,#0x1a0]
    ldr w4,[sp,#0x110]
    bl lab_director_sum_callback
    str w0,[sp,#0x100]
    restore_context
.global lab_director_sum_return
lab_director_sum_return:
    b .
.size lab_director_sum_hook,.-lab_director_sum_hook
