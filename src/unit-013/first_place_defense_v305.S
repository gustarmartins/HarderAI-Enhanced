#include "register_context.inc"
.text
.balign 4
.global lab_first_place_hold_hook
.type lab_first_place_hold_hook,%function
lab_first_place_hold_hook:
    save_context
    mov x0,x19
    mov w1,wzr
    bl lab_first_place_protection
    cbz w0,.Lhold_native
    restore_context
    mov w8,#100
    b lab_first_place_hold_return
.Lhold_native:
    restore_context
    ldr w8,[x8,#0x118]
.global lab_first_place_hold_return
lab_first_place_hold_return:
    b .
.size lab_first_place_hold_hook,.-lab_first_place_hold_hook

/* Keep already trailing protection instead of requesting an offensive throw.
 * Also clears an earlier periodic-use latch; the normal held output remains. */
.global lab_first_place_snipe_hook
.type lab_first_place_snipe_hook,%function
lab_first_place_snipe_hook:
    save_context
    mov w1,#1
    bl lab_first_place_protection
    cbz w0,.Lsnipe_native
    restore_context
    strb wzr,[x0,#0xb8]
    ret
.Lsnipe_native:
    restore_context
    sub sp,sp,#0x50
.global lab_first_place_snipe_return
lab_first_place_snipe_return:
    b .
.size lab_first_place_snipe_hook,.-lab_first_place_snipe_hook

/* Native pending-action consumers reject direction3. This cancels only the
 * expired default throw choice while the same defensive item is trailing. */
.global lab_first_place_timeout_hook
.type lab_first_place_timeout_hook,%function
lab_first_place_timeout_hook:
    save_context
    mov w1,#1
    bl lab_first_place_protection
    cbz w0,.Ltimeout_native
    restore_context
    mov w9,#3
    str w9,[x8]
    ret
.Ltimeout_native:
    restore_context
    sub sp,sp,#0x30
.global lab_first_place_timeout_return
lab_first_place_timeout_return:
    b .
.size lab_first_place_timeout_hook,.-lab_first_place_timeout_hook
