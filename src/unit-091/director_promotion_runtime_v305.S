#include "register_context.inc"
.text
.balign 4
.global lab_director_promotion_hook
.type lab_director_promotion_hook,%function
lab_director_promotion_hook:
    /* The native recursive walk supplies a live x19 object. Reject other
     * object types before the full save; EOR/CBNZ leave native NZCV intact. */
    stp x8,x9,[sp,#-16]!
    adrp x8,lab_module_start
    add x8,x8,:lo12:lab_module_start
    ldr x9,=(LAB_MAIN_TO_MODULE-0x11ad808)
    sub x8,x8,x9
    ldr x9,[x19]
    eor x9,x8,x9
    cbnz x9,.Lpromotion_skip
    ldr x8,[x19,#0x40]
    add x8,x8,#8
    eor x9,x8,x21
    cbnz x9,.Lpromotion_skip
    ldr w8,[x19,#0x38]
    eor x9,x8,x20
    cbnz x9,.Lpromotion_skip
    ldp x8,x9,[sp],#16
    save_context
    mov x0,sp
    bl lab_director_promotion_dispatch
    restore_context
    b .Lpromotion_replay
.Lpromotion_skip:
    ldp x8,x9,[sp],#16
.Lpromotion_replay:
    sub x20,x20,#1
.global lab_director_promotion_return
lab_director_promotion_return:
    b .
.size lab_director_promotion_hook,.-lab_director_promotion_hook
