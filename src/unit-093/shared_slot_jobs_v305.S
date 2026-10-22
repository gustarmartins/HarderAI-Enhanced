#include "register_context.inc"
#include "shared_slot_jobs_v305.h"
.text
.balign 4
.global lab_shared_jobs_pre_hook
.type lab_shared_jobs_pre_hook,%function
lab_shared_jobs_pre_hook:
    /* The retail site uses BL, retaining its exact 13E5B8 return address.
     * Native X4=N+8 and X5=N remain pointers into the ORIGINAL frame. */
    sub sp,sp,#LAB_SHARED_JOB_FRAME_BYTES
    save_context
    mov x0,sp
    add x1,sp,#0x320
    add x2,x1,#LAB_SHARED_JOB_FRAME_BYTES
    bl lab_shared_jobs_prepare
    restore_context
    /* Keep the receipt allocated through all three native calls. */
.global lab_shared_jobs_submit
lab_shared_jobs_submit:
    b .
.size lab_shared_jobs_pre_hook,.-lab_shared_jobs_pre_hook

.balign 4
.global lab_shared_jobs_post_hook
.type lab_shared_jobs_post_hook,%function
lab_shared_jobs_post_hook:
    /* Save POST-call state. Reinstating the pre-call snapshot would erase
     * legitimate native register/FP results. Cleanup is admission-independent. */
    save_context
    mov x0,sp
    add x1,sp,#0x320
    add x2,x1,#LAB_SHARED_JOB_FRAME_BYTES
    bl lab_shared_jobs_finish
    restore_context
    add sp,sp,#LAB_SHARED_JOB_FRAME_BYTES
    ldp x29,x30,[sp,#0x30]
.global lab_shared_jobs_return
lab_shared_jobs_return:
    b .
.size lab_shared_jobs_post_hook,.-lab_shared_jobs_post_hook
