/* Uninstalled v3.0.5 local-selection hooks. Build supplies main-relative
 * continuation/retail symbols, native-owned inputs address, and an abort
 * continuation. Abort integration must be qualified before installation.
 * Inputs: { u64 main_base, epoch, IdentityLocal *scope }. */
#define hook_inputs lab_identity_local_hook_inputs
#define abort_local lab_identity_local_abort
#define racer_resume lab_identity_local_racer_resume
#define racer_retail lab_identity_local_racer_retail
#define type_resume lab_identity_local_type_resume
#ifndef LAB_IDENTITY_LOCAL_PLAYERS
#define LAB_IDENTITY_LOCAL_PLAYERS 1
#endif

.macro save_all
    sub sp, sp, #0x320
    stp x0, x1, [sp, #0x00]
    stp x2, x3, [sp, #0x10]
    stp x4, x5, [sp, #0x20]
    stp x6, x7, [sp, #0x30]
    stp x8, x9, [sp, #0x40]
    stp x10, x11, [sp, #0x50]
    stp x12, x13, [sp, #0x60]
    stp x14, x15, [sp, #0x70]
    stp x16, x17, [sp, #0x80]
    stp x18, x19, [sp, #0x90]
    stp x20, x21, [sp, #0xa0]
    stp x22, x23, [sp, #0xb0]
    stp x24, x25, [sp, #0xc0]
    stp x26, x27, [sp, #0xd0]
    stp x28, x29, [sp, #0xe0]
    str x30, [sp, #0xf0]
    stp q0, q1, [sp, #0x100]
    stp q2, q3, [sp, #0x120]
    stp q4, q5, [sp, #0x140]
    stp q6, q7, [sp, #0x160]
    stp q8, q9, [sp, #0x180]
    stp q10, q11, [sp, #0x1a0]
    stp q12, q13, [sp, #0x1c0]
    stp q14, q15, [sp, #0x1e0]
    stp q16, q17, [sp, #0x200]
    stp q18, q19, [sp, #0x220]
    stp q20, q21, [sp, #0x240]
    stp q22, q23, [sp, #0x260]
    stp q24, q25, [sp, #0x280]
    stp q26, q27, [sp, #0x2a0]
    stp q28, q29, [sp, #0x2c0]
    stp q30, q31, [sp, #0x2e0]
    mrs x9, nzcv
    str x9, [sp, #0x300]
    mrs x9, fpcr
    str x9, [sp, #0x308]
    mrs x9, fpsr
    str x9, [sp, #0x310]
#ifdef LAB_IDENTITY_STORAGE_GUARDS
    bl lab_identity_storage_hook_enter
    /* Existing padding; keep native SP and saved-caller offsets unchanged. */
    str x0, [sp, #0xf8]
    str x1, [sp, #0x318]
    reload_all
#endif
.endm

.macro reload_all
    ldr x9, [sp, #0x300]
    msr nzcv, x9
    ldr x9, [sp, #0x308]
    msr fpcr, x9
    ldr x9, [sp, #0x310]
    msr fpsr, x9
    ldp q0, q1, [sp, #0x100]
    ldp q2, q3, [sp, #0x120]
    ldp q4, q5, [sp, #0x140]
    ldp q6, q7, [sp, #0x160]
    ldp q8, q9, [sp, #0x180]
    ldp q10, q11, [sp, #0x1a0]
    ldp q12, q13, [sp, #0x1c0]
    ldp q14, q15, [sp, #0x1e0]
    ldp q16, q17, [sp, #0x200]
    ldp q18, q19, [sp, #0x220]
    ldp q20, q21, [sp, #0x240]
    ldp q22, q23, [sp, #0x260]
    ldp q24, q25, [sp, #0x280]
    ldp q26, q27, [sp, #0x2a0]
    ldp q28, q29, [sp, #0x2c0]
    ldp q30, q31, [sp, #0x2e0]
    ldp x0, x1, [sp, #0x00]
    ldp x2, x3, [sp, #0x10]
    ldp x4, x5, [sp, #0x20]
    ldp x6, x7, [sp, #0x30]
    ldp x8, x9, [sp, #0x40]
    ldp x10, x11, [sp, #0x50]
    ldp x12, x13, [sp, #0x60]
    ldp x14, x15, [sp, #0x70]
    ldp x16, x17, [sp, #0x80]
    ldp x18, x19, [sp, #0x90]
    ldp x20, x21, [sp, #0xa0]
    ldp x22, x23, [sp, #0xb0]
    ldp x24, x25, [sp, #0xc0]
    ldp x26, x27, [sp, #0xd0]
    ldp x28, x29, [sp, #0xe0]
    ldr x30, [sp, #0xf0]
.endm

.macro restore_all
#ifdef LAB_IDENTITY_STORAGE_GUARDS
    ldr x0, [sp, #0xf8]
    ldr x1, [sp, #0x318]
    bl lab_identity_storage_hook_leave
#endif
    reload_all
    add sp, sp, #0x320
.endm

.macro load_local_inputs reg
#ifdef LAB_IDENTITY_ROUTED
    bl lab_identity_local_inputs_current
    mov \reg, x0
#else
    adrp \reg, hook_inputs
    add \reg, \reg, :lo12:hook_inputs
#endif
.endm

#ifndef LAB_IDENTITY_REGISTER_MACROS_ONLY
.macro select field, index_offset
    load_local_inputs x9
    ldp x0, x1, [x9]
    ldr x2, [x9, #16]
    mov w3, #\field
    ldr w4, [sp, #\index_offset]
    bl lab_identity_local_select
.endm

.section .text.lab_identity_local_racer_hook,"ax",@progbits
.global lab_identity_local_racer_hook
lab_identity_local_racer_hook:
    save_all
    select 0, 8
    cmn w0, #1
    b.eq .Lracer_retail
#if LAB_IDENTITY_LOCAL_PLAYERS == 2
    cmp w0, #LAB_IDENTITY_LOCAL_PLAYERS
    b.hs .Lracer_fault
    str x0, [sp]  /* Zero-extended selected racer, distinct from input slot. */
#else
    cmp w0, #0
    b.ne .Lracer_fault
    str xzr, [sp]  /* Active P1 result, including zero-extended X0. */
#endif
    adrp x9, racer_resume
    add x9, x9, :lo12:racer_resume
    str x9, [sp, #0xf0] /* Emulate the displaced BL's link register. */
    restore_all
    b racer_resume
.Lracer_retail:
    adrp x9, racer_resume
    add x9, x9, :lo12:racer_resume
    str x9, [sp, #0xf0]
    restore_all
    b racer_retail /* Native getter returns directly to the original site. */
.Lracer_fault:
    restore_all
    b abort_local

.section .text.lab_identity_local_type_hook,"ax",@progbits
.global lab_identity_local_type_hook
lab_identity_local_type_hook:
    save_all
    select 1, 0
    cmn w0, #1
    b.eq .Ltype_retail
    cmp w0, #1
    b.ne .Ltype_fault
    mov x9, #1
    str x9, [sp, #0x40]
    restore_all
    b type_resume
.Ltype_retail:
    restore_all
    cinc w8, w28, ne
    b type_resume
.Ltype_fault:
    restore_all
    b abort_local

#if LAB_IDENTITY_LOCAL_PLAYERS == 2
/* Native 7F4468 normally copies the human-role count into W21. Retain the
 * separately owned two physical seats after P1 becomes CPU. Inactive routes
 * execute the original MOV. This site does not replace a call or set LR. */
.section .text.lab_identity_local_count_hook,"ax",@progbits
.global lab_identity_local_count_hook
lab_identity_local_count_hook:
    save_all
    load_local_inputs x9
    ldp x0, x1, [x9]
    ldr x2, [x9, #16]
    mov w3, #2
    mov w4, wzr
    bl lab_identity_local_select
    cmn w0, #1
    b.eq .Lcount_retail
    cmp w0, #2
    b.ne .Lcount_fault
    str x0, [sp, #0xa8]
    restore_all
    b lab_identity_local_count_resume
.Lcount_retail:
    restore_all
    mov w21, w0
    b lab_identity_local_count_resume
.Lcount_fault:
    restore_all
    b abort_local
#endif

#ifdef LAB_IDENTITY_LOCAL_EVENTS
/* Scope opens at the real mode-3 builder call, after native record reset.
 * Scope becomes READY after the final map rebuild, before virtual +70.
 * Fault targets still require a qualified outer unwind/unlock contract. */
.section .text.lab_identity_local_enter_hook,"ax",@progbits
.global lab_identity_local_enter_hook
lab_identity_local_enter_hook:
    save_all
    load_local_inputs x0
    ldr x1, [sp]
    bl lab_identity_local_enter_event
    cmp w0, #1
    b.hi .Lenter_fault
    adrp x9, lab_identity_local_enter_resume
    add x9, x9, :lo12:lab_identity_local_enter_resume
    str x9, [sp, #0xf0]
    restore_all
    b lab_identity_local_builder
.Lenter_fault:
    restore_all
    b abort_local

.section .text.lab_identity_local_ready_hook,"ax",@progbits
.global lab_identity_local_ready_hook
lab_identity_local_ready_hook:
    save_all
    load_local_inputs x0
    ldr x1, [sp, #0x98] /* Original X19: native local-manager receiver. */
    bl lab_identity_local_ready_event
    cmp w0, #1
    b.hi .Lready_fault
    restore_all
    ldr x8, [x19] /* Displaced instruction at 7F4850. */
    b lab_identity_local_ready_resume
.Lready_fault:
    restore_all
    b abort_local
#endif
#endif /* LAB_IDENTITY_REGISTER_MACROS_ONLY */
