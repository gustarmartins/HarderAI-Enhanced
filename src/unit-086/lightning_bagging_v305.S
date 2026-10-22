#include "register_context.inc"
.text
.balign 4
.global lab_lightning_picker_entry_hook
.type lab_lightning_picker_entry_hook,%function
lab_lightning_picker_entry_hook:
    sub sp,sp,#0x180
    /* Exact retail46300 frame leaves[0..7] unused. No borrowed state escapes. */
    str w2,[sp]
.global lab_lightning_picker_entry_return
lab_lightning_picker_entry_return:
    b .
.size lab_lightning_picker_entry_hook,.-lab_lightning_picker_entry_hook

.global lab_lightning_curve_hook
.type lab_lightning_curve_hook,%function
lab_lightning_curve_hook:
    sub sp,sp,#0x20
    stp x0,x8,[sp]
    str s0,[sp,#0x10]
    blr x8
    save_context
    mov x0,sp
    add x1,sp,#0x320
    add x2,sp,#0x340
    bl lab_lightning_curve_finish
    str w0,[sp,#0x100]
    restore_context
    add sp,sp,#0x20
.global lab_lightning_curve_return
lab_lightning_curve_return:
    b .
.size lab_lightning_curve_hook,.-lab_lightning_curve_hook

.global lab_lightning_delay_hook
.type lab_lightning_delay_hook,%function
lab_lightning_delay_hook:
    save_context
    mov x0,x19
    mov w1,w8
    bl lab_lightning_first_delay
    str w0,[sp,#0x40]
    restore_context
    str w8,[x19,#0xec]
.global lab_lightning_delay_return
lab_lightning_delay_return:
    b .
.size lab_lightning_delay_hook,.-lab_lightning_delay_hook
