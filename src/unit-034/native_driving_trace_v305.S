#include "register_context.inc"
.text
.balign 4
.global lab_native_driving_trace_hook
.type lab_native_driving_trace_hook,%function
lab_native_driving_trace_hook:
    save_context
    mov x0,x19
    mov w1,w20
    mov w2,w21
    mov w3,w22
    mov w4,w23
    mov w5,w24
    mov w6,w25
    /* s0 still contains the native final dot, including its original bits. */
    bl lab_native_driving_trace_exit
    restore_context
    /* Native 0x5D3EC, exactly once and after passive capture. */
    ldrb w8,[x19,#0x41]
.global lab_native_driving_trace_return
lab_native_driving_trace_return:
    b .
.size lab_native_driving_trace_hook,.-lab_native_driving_trace_hook
