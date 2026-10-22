#include "register_context.inc"
.text
.balign 4
.global lab_golden_active_narrow_hook
.type lab_golden_active_narrow_hook,%function
lab_golden_active_narrow_hook:
    /* Native TST W0,#1 has already run. Wide routes keep the displaced
     * CSINC exactly. This seam follows countdown and the other native gates. */
    tbz w0,#0,.Lgolden_narrow_native
    save_context
    mov x0,x19
    bl lab_golden_repeat_allowed
    cbz w0,.Lgolden_narrow_restore
    restore_context
    /* The Golden lifetime is already running. Request its next native use;
     * inventory, consumption, duration and the six-update retry stay native. */
    mov w20,#1
    b lab_golden_active_narrow_return
.Lgolden_narrow_restore:
    restore_context
.Lgolden_narrow_native:
    csinc w20,w20,wzr,ne
.global lab_golden_active_narrow_return
lab_golden_active_narrow_return:
    b .
.size lab_golden_active_narrow_hook,.-lab_golden_active_narrow_hook

.section .rodata,"a",@progbits
.balign 4
.global lab_golden_active_narrow_config
.type lab_golden_active_narrow_config,%object
lab_golden_active_narrow_config:
    .word 1,0x62228,70,6
.size lab_golden_active_narrow_config,.-lab_golden_active_narrow_config
