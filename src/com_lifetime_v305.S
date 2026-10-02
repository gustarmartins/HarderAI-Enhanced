#include "register_context.inc"
#ifndef LAB_MAIN_TO_MODULE
#error Main-to-module placement must be explicit
#endif
.text
.balign 4
.macro lifetime kind, object, selector=8
    save_context
    adrp x0,lab_com_lifetime_record
    add x0,x0,:lo12:lab_com_lifetime_record
    adrp x1,lab_module_start
    add x1,x1,:lo12:lab_module_start
    movz x9,#(LAB_MAIN_TO_MODULE & 0xffff)
    movk x9,#((LAB_MAIN_TO_MODULE >> 16) & 0xffff),lsl #16
    sub x1,x1,x9
    mov w2,#\kind
    ldr x3,[sp,#(8*\object)]
    ldr x4,[sp,#(8*\selector)]
    bl lab_com_lifetime_update
    restore_context
.endm
.global lab_com_lifetime_publish_hook
lab_com_lifetime_publish_hook:
    lifetime 1,19
    add x8,x9,#0x10
.global lab_com_lifetime_publish_return
lab_com_lifetime_publish_return:
    b .
.global lab_com_lifetime_retire_hook
lab_com_lifetime_retire_hook:
    lifetime 2,20
    ldr x8,[x20]
.global lab_com_lifetime_retire_return
lab_com_lifetime_retire_return:
    b .
.global lab_com_lifetime_begin_hook
lab_com_lifetime_begin_hook:
    lifetime 3,0
    sub sp,sp,#0xc0
.global lab_com_lifetime_begin_return
lab_com_lifetime_begin_return:
    b .
.global lab_com_lifetime_group_hook
lab_com_lifetime_group_hook:
    str x20,[x19,#0xb0]
    lifetime 4,19,20
.global lab_com_lifetime_group_return
lab_com_lifetime_group_return:
    b .
.global lab_com_lifetime_destroy_hook
lab_com_lifetime_destroy_hook:
    lifetime 5,0
    stp x20,x19,[sp,#-0x20]!
.global lab_com_lifetime_destroy_return
lab_com_lifetime_destroy_return:
    b .
.global lab_com_lifetime_delete_hook
lab_com_lifetime_delete_hook:
    lifetime 5,0
    stp x20,x19,[sp,#-0x20]!
.global lab_com_lifetime_delete_return
lab_com_lifetime_delete_return:
    b .
