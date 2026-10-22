/* Offline single-player GP/VS completion predicate, inside retail879C2C..D90.
 * No persistent state, role writes, clock writes or external module dependency.
 * Native per-racer finish bit2 and timestamp+48 remain authoritative.
 * Ordinary native timeout remains9:59.999. A sole local human may finish first;
 * others have30 race-clock seconds to finish before native finalization.
 * Other modes/rosters retain native subject selection. This companion must not
 * be combined with the private two-physical-seat identity finish adapter.
 */
.section .text.lab_all_racers_finish_v305,"ax",@progbits
.global lab_all_racers_finish_v305
.type lab_all_racers_finish_v305,%function
lab_all_racers_finish_v305:
    stp x19, x30, [sp, #-16]!
    bl finish_native_context
    cbz x0, .Lno
    ldr x19, [x0, #0x1b0]
    cbz x19, .Lno
    ldr x19, [x19, #0x218]
    cbz x19, .Lno
    bl finish_native_config
    ldr x1, [x19, #0xe0]
    ldrb w2, [x1, #0x44]
    mov w3, #60000
    mul w2, w2, w3
    ldrb w4, [x1, #0x45]
    mov w5, #1000
    madd w2, w4, w5, w2
    ldrh w4, [x1, #0x46]
    add w2, w2, w4
    mov w4, #0x27bf
    movk w4, #9, lsl #16
    cmp w2, w4
    b.hs .Lyes

    ldrsw x8, [x0, #0x180]
    cmp w8, #1
    b.lt .Lyes
    ldr w9, [x0, #0x184]
    mov w16, wzr
    cmp w9, #1
    b.ne .Lloop_setup
    ldr w10, [x0, #0x188]
    cmp w10, #1
    b.ne .Lloop_setup
    ldr x10, [x0] /* config network/mode-of-play words0 and4 must both be0 */
    cbnz x10, .Lloop_setup
    ldr w10, [x0, #8]
    cmp w10, #1
    b.hi .Lloop_setup
    mov w16, #1
.Lloop_setup:
    ldr x1, [x19, #0x68]
    mov x6, xzr
    mov w7, wzr /* unfinished total */
    mov x15, xzr /* finished human's current native record */
.Lloop:
    cmp x6, #12
    csel x11, x6, xzr, lo
    mov w13, #28
    madd x11, x11, x13, x0
    ldr w11, [x11, #0x44]
    bic w11, w11, #2
    cbnz w16, .Lread_record
    cmp w9, #0
    b.le .Lread_record
    cbnz w11, .Lnext
.Lread_record:
    ldr x12, [x1, x6, lsl #3]
    ldrb w10, [x12, #0x3c]
    tbz w10, #2, .Lunfinished
    cbnz w11, .Lnext
    mov x15, x12
    b .Lnext
.Lunfinished:
    cbz w16, .Lno
    cbz w11, .Lno /* the sole human itself is still racing */
    mov w7, #1
.Lnext:
    add x6, x6, #1
    cmp x6, x8
    b.lt .Lloop
    cbz w16, .Lyes
    cbz w7, .Lyes
    cbz x15, .Lno
    ldrb w10, [x15, #0x4c]
    mul w10, w10, w3
    ldrb w11, [x15, #0x4d]
    madd w10, w11, w5, w10
    ldrh w11, [x15, #0x4e]
    add w10, w10, w11
    subs w10, w2, w10
    b.lo .Lno /* future/sentinel timestamp must not underflow into timeout */
    mov w11, #30000
    cmp w10, w11
    b.lo .Lno
.Lyes:
    mov w0, #1
    b .Lreturn
.Lno:
    mov w0, wzr
.Lreturn:
    ldp x19, x30, [sp], #16
    ret
.size lab_all_racers_finish_v305, .-lab_all_racers_finish_v305
