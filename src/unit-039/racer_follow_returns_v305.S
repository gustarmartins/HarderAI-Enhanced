/* Production module veneers only. Patch each instruction from the exact
 * RETURNS map in racer_follow_guard_v305.py before packaging. The isolated
 * proof instead binds these names with --defsym and must omit this file. */
.text
.p2align 2
.global lab_follow_native_update
lab_follow_native_update:
    .word 0x14000000
.global lab_follow_native_writer
lab_follow_native_writer:
    .word 0x14000000
.global lab_follow_native_abort
lab_follow_native_abort:
    .word 0x14000000
.global lab_follow_native_after_target
lab_follow_native_after_target:
    .word 0x14000000
.global lab_follow_native_after_secondary
lab_follow_native_after_secondary:
    .word 0x14000000
.global lab_follow_native_after_update
lab_follow_native_after_update:
    .word 0x14000000
