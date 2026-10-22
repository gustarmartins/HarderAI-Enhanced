#include "register_context.inc"
.text
.balign 4
.macro golden_gate name, resume, blocked
.global \name
.type \name,%function
\name:
    tbz w0,#0,\resume
    save_context
    mov x0,x19
    bl lab_golden_repeat_allowed
    cbz w0,1f
    restore_context
    b \resume
1:
    restore_context
.global \blocked
\blocked:
    b .
.global \resume
\resume:
    b .
.size \name,.-\name
.endm
golden_gate lab_golden_route_gate,lab_golden_route_return,lab_golden_route_blocked
golden_gate lab_golden_ordinary_gate,lab_golden_ordinary_return,lab_golden_ordinary_blocked

.global lab_golden_delay_hook
.type lab_golden_delay_hook,%function
lab_golden_delay_hook:
    mov w8,#6
.global lab_golden_delay_return
lab_golden_delay_return:
    b .
.size lab_golden_delay_hook,.-lab_golden_delay_hook

/* A Golden already spent its first use: its lifetime now advances whether
 * the CPU presses again or waits. Reuse the native ordinary six-update retry
 * instead of waiting for the shortcut's route-point admission. Unused Golden
 * and every other held item retain that route decision. */
.balign 4
.global lab_golden_shortcut_retry_hook
.type lab_golden_shortcut_retry_hook,%function
lab_golden_shortcut_retry_hook:
    save_context
    mov x0,x19
    bl lab_golden_repeat_allowed
    cbnz w0,.Lgolden_shortcut_repeat
    mov x0,x19
    bl lab_golden_first_use_urgent
    cbnz w0,.Lboost_finish_urgent
    mov x0,x19
    bl lab_star_first_use_urgent
    cbz w0,.Lgolden_shortcut_native
.Lboost_finish_urgent:
    restore_context
    /* Eligible Golden or Star near the finish: expire only the waiting countdown.
     * The ordinary native safety/boost/route/item-consumption gates follow. */
    str wzr,[x19,#0xec]
    mov w20,wzr
    b lab_golden_shortcut_ordinary_return
.Lgolden_shortcut_repeat:
    restore_context
    /* W20 otherwise permits a separate shortcut emergency-use path, even
     * before the retry countdown elapses. Keep this repetition paced. */
    mov w20,wzr
    b lab_golden_shortcut_ordinary_return
.Lgolden_shortcut_native:
    restore_context
    cbz w8,lab_golden_shortcut_ordinary_return
.global lab_golden_shortcut_retry_return
lab_golden_shortcut_retry_return:
    b .
.global lab_golden_shortcut_ordinary_return
lab_golden_shortcut_ordinary_return:
    b .
.size lab_golden_shortcut_retry_hook,.-lab_golden_shortcut_retry_hook

.section .rodata,"a",@progbits
.balign 4
.global lab_golden_shortcut_retry_config
.type lab_golden_shortcut_retry_config,%object
lab_golden_shortcut_retry_config:
    .word 1,6,0x62170,0x621c0
.size lab_golden_shortcut_retry_config,.-lab_golden_shortcut_retry_config
