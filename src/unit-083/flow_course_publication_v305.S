#include "register_context.inc"
.text
.balign 4
.macro course_event kind, object
    save_context
    adrp x0,lab_flow_course_publication
    add x0,x0,:lo12:lab_flow_course_publication
    adrp x1,lab_module_start
    add x1,x1,:lo12:lab_module_start
    movz x9,#(LAB_MAIN_TO_MODULE & 0xffff)
    movk x9,#((LAB_MAIN_TO_MODULE >> 16) & 0xffff),lsl #16
    sub x1,x1,x9
    mov w2,#\kind
    ldr x3,[sp,#(8*\object)]
    bl lab_flow_course_event
    restore_context
.endm
.global lab_flow_course_begin_hook
lab_flow_course_begin_hook:
    course_event 1,0
    sub sp,sp,#0x160
.global lab_flow_course_begin_return
lab_flow_course_begin_return: b .
.global lab_flow_course_publish_hook
lab_flow_course_publish_hook:
    course_event 2,19
.global lab_flow_course_publish_return
lab_flow_course_publish_return: b .
.global lab_flow_course_retire_hook
lab_flow_course_retire_hook:
    course_event 3,0
    str x19,[sp,#-0x20]!
.global lab_flow_course_retire_return
lab_flow_course_retire_return: b .
.global lab_flow_course_reload_hook
lab_flow_course_reload_hook:
    course_event 4,0
    sub sp,sp,#0x70
.global lab_flow_course_reload_return
lab_flow_course_reload_return: b .
.global lab_flow_course_wrapper_delete_hook
lab_flow_course_wrapper_delete_hook:
    course_event 5,0
    str x19,[sp,#-0x20]!
.global lab_flow_course_wrapper_delete_return
lab_flow_course_wrapper_delete_return: b .
.global lab_flow_course_factory_reset_hook
lab_flow_course_factory_reset_hook:
    course_event 6,0
    str x19,[sp,#-0x20]!
.global lab_flow_course_factory_reset_return
lab_flow_course_factory_reset_return: b .
.global lab_flow_course_data_delete_hook
lab_flow_course_data_delete_hook:
    course_event 7,0
.global lab_flow_course_data_delete_return
lab_flow_course_data_delete_return: b .
.global lab_flow_course_factory_release_hook
lab_flow_course_factory_release_hook:
    course_event 8,0
    str x19,[sp,#-0x20]!
.global lab_flow_course_factory_release_return
lab_flow_course_factory_release_return: b .
