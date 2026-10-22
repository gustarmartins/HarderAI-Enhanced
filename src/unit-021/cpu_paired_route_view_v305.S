#include "register_context.inc"
.text
.balign 4
.macro paired_view_store name,retname,hold
.global \name
.type \name,%function
\name:
    save_context
    mov x1,x0
    mov x0,x19
    mov x2,x20
    adrp x3,lab_module_start
    add x3,x3,:lo12:lab_module_start
    movz x9,#(LAB_MAIN_TO_MODULE & 0xffff)
    movk x9,#((LAB_MAIN_TO_MODULE >> 16) & 0xffff),lsl #16
    sub x3,x3,x9
    bl lab_cpu_paired_route_view
    .if \hold
    mov x1,x0
    mov x0,x19
    mov x2,x20
    adrp x3,lab_module_start
    add x3,x3,:lo12:lab_module_start
    movz x9,#(LAB_MAIN_TO_MODULE & 0xffff)
    movk x9,#((LAB_MAIN_TO_MODULE >> 16) & 0xffff),lsl #16
    sub x3,x3,x9
    bl lab_snaking_route_hold
    .endif
    str x0,[sp,#0]
    restore_context
    str x0,[x19,#0x80]
.global \retname
\retname:
    b .
.size \name,.-\name
.endm
paired_view_store lab_cpu_paired_view_initial_hook,lab_cpu_paired_view_initial_return,0
paired_view_store lab_cpu_paired_view_update_hook,lab_cpu_paired_view_update_return,1
