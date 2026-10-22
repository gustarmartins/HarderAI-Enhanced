/* Isolated passive hooks. Full native target block executes exactly once. */
#include "register_context.inc"
.text
.balign 4
.global lab_flow_lane_target_hook
.type lab_flow_lane_target_hook,%function
lab_flow_lane_target_hook:
    save_context
    mov x0,x19
    mov w1,w21
    mov w2,w22
    mov w3,w23
    add x4,sp,#0x320
    bl lab_flow_lane_runtime_entry
    restore_context
    /* Exact retail7BE418..7BE448. Read actual stored result afterwards. */
    ldp s0,s1,[x19,#0x94]
    fmul s0,s1,s0
    ldp s2,s3,[x19,#0x8c]
    fmul s2,s1,s2
    fmul s3,s1,s3
    fmov s1,w21
    fadd s1,s1,s2
    fmov s2,w22
    fadd s2,s2,s3
    stp s1,s2,[x19,#0x9c]
    fmov s1,w23
    fadd s0,s1,s0
    str s0,[x19,#0xa4]
    save_context
    mov x0,x19
    mov w1,w21
    mov w2,w22
    mov w3,w23
    bl lab_flow_lane_target_publish
    restore_context
.global lab_flow_lane_target_return
lab_flow_lane_target_return:
    b .
.size lab_flow_lane_target_hook,.-lab_flow_lane_target_hook

.balign 4
.global lab_flow_lane_contact_hook
.type lab_flow_lane_contact_hook,%function
lab_flow_lane_contact_hook:
    save_context
    mov x0,x19
    mov x1,x27
    mov w2,w24
    add x3,sp,#0x320
    bl lab_flow_lane_contact_publish
    restore_context
    /* Displaced native189118. Native stack and flags are restored first. */
    ldr s0,[x19,#0x1dc]
.global lab_flow_lane_contact_return
lab_flow_lane_contact_return:
    b .
.size lab_flow_lane_contact_hook,.-lab_flow_lane_contact_hook
