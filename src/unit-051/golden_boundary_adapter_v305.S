/* Private Golden observer ABI. These wrappers never modify game policy.
 * Only the displaced instruction may modify native state. Scalar publication
 * follows the native use store; all register snapshots retain its operands.
 */
.text
.p2align 2
#include "register_context.inc"

.macro boundary event
    save_context
    mov w0,#\event
    mov x1,sp
    add x2,sp,#0x320
    bl lab_golden_boundary_dispatch
    restore_context
.endm
.macro scalar event, previous=-1
    save_context
    mov w0,#\event
    mov x1,sp
    add x2,sp,#0x320
    .if \previous >= 0
    ldr w3,[sp,#\previous]
    .else
    mov w3,wzr
    .endif
    bl lab_golden_scalar_dispatch
    restore_context
.endm
.macro resume name
    .global lab_golden_\name\()_return
lab_golden_\name\()_return:
    .inst 0x14000000
.endm

.global lab_golden_begin_hook
.type lab_golden_begin_hook,%function
lab_golden_begin_hook:
    boundary 1
    ldrb w8,[x19,#0x170]
    resume begin
.size lab_golden_begin_hook,.-lab_golden_begin_hook

/* All active command paths converge at353C, after both optional worker
 * batches and the pause test, before either command-side virtual call. */
.global lab_golden_commands_hook
.type lab_golden_commands_hook,%function
lab_golden_commands_hook:
    boundary 2
    ldr x0,[x19,#0x100]
    resume commands
.size lab_golden_commands_hook,.-lab_golden_commands_hook

.global lab_golden_end_hook
.type lab_golden_end_hook,%function
lab_golden_end_hook:
    boundary 3
    ldp x29,x30,[sp,#0x90]
    resume end
.size lab_golden_end_hook,.-lab_golden_end_hook

.global lab_golden_use_a_hook
.type lab_golden_use_a_hook,%function
lab_golden_use_a_hook:
    save_context
    /* W10 is450 on first Golden use. Native code has already initialized
     * [X8] to0; capture that count immediately before its publication. */
    ldr w3,[x8]
    ldr w9,[sp,#0x48]
    str w9,[x8]
    mov w0,#4
    mov x1,sp
    add x2,sp,#0x320
    bl lab_golden_scalar_dispatch
    restore_context
    resume use_a
.size lab_golden_use_a_hook,.-lab_golden_use_a_hook

.global lab_golden_use_b_hook
.type lab_golden_use_b_hook,%function
lab_golden_use_b_hook:
    str w9,[x8]
    scalar 5,0xc0 /* original W24 */
    resume use_b
.size lab_golden_use_b_hook,.-lab_golden_use_b_hook

.global lab_golden_tick_hook
.type lab_golden_tick_hook,%function
lab_golden_tick_hook:
    scalar 6
    ldp x29,x30,[sp,#0x20]
    resume tick
.size lab_golden_tick_hook,.-lab_golden_tick_hook

/* B.LT consults the incoming native CMP flags before entering C. The
 * observer sees only taken GE; the other arm reaches the native tick
 * epilogue exactly once. The expiry tail never returns through3FDD0. */
.global lab_golden_expiry_hook
.type lab_golden_expiry_hook,%function
lab_golden_expiry_hook:
    b.lt 1f
    scalar 7
    resume expiry_taken
1:  resume expiry_next
.size lab_golden_expiry_hook,.-lab_golden_expiry_hook

.global lab_golden_limit_hook
.type lab_golden_limit_hook,%function
lab_golden_limit_hook:
    b.lt 1f
    scalar 8
    resume limit_taken
1:  resume limit_next
.size lab_golden_limit_hook,.-lab_golden_limit_hook
