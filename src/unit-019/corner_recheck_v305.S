#include "register_context.inc"
.text
.balign 4
.global lab_corner_recheck_hook
.type lab_corner_recheck_hook,%function
lab_corner_recheck_hook:
    ldr w20,[x8,#0xa0]
    save_context
    mov x0,x19
    mov w1,w20
    bl lab_corner_recheck_phase
    /* Original LDR W20 cleared the upper half of saved X20. */
    str w0,[sp,#0xa0]
    restore_context
.global lab_corner_recheck_return
lab_corner_recheck_return:
    b .
.size lab_corner_recheck_hook,.-lab_corner_recheck_hook
