/* Adjust only the existing6/7-state native charge call, until blue is earned.
 * This does not enter a drift, hold a route, release a boost or award a tier. */
.text
.balign 4
.global lab_held_blue_charge_hook
.type lab_held_blue_charge_hook,%function
lab_held_blue_charge_hook:
    mov v1.16b,v8.16b
    sub sp,sp,#48
    stp x9,x10,[sp]
    str q3,[sp,#16]
    mrs x9,nzcv
    str x9,[sp,#32]
    mrs x9,fpsr
    str x9,[sp,#40]
    ldr w9,[x19,#0x8c]
    cbnz w9,.Lheld_done
    ldr w9,[x19,#0x10]
    sub w9,w9,#6
    cmp w9,#1
    b.hi .Lheld_done
    ldr x9,[x19,#8]
    cbz x9,.Lheld_done
    ldrb w10,[x9,#0xd2]
    cmp w10,#1
    b.ne .Lheld_done
    ldrb w10,[x9,#0xd3]
    cmp w10,#1
    b.ne .Lheld_done
    ldrb w10,[x9,#0xe1]
    cbz w10,.Lheld_done
    ldrb w10,[x9,#0xe7]
    cbnz w10,.Lheld_done
    ldr w10,[x9,#0x1cc]
    mov w9,#0x4018
    movk w9,#0x120,lsl#16
    tst w10,w9
    b.ne .Lheld_done
    /* Only the ordinary0.40 scale belongs to this intervention. */
    adrp x9,lab_held_blue_native_scale
    ldr s3,[x9,:lo12:lab_held_blue_native_scale]
    fcmp s1,s3
    b.ne .Lheld_done
    adrp x9,lab_held_blue_charge_scale
    ldr s1,[x9,:lo12:lab_held_blue_charge_scale]
.Lheld_done:
    ldr x9,[sp,#40]
    msr fpsr,x9
    ldr x9,[sp,#32]
    msr nzcv,x9
    ldr q3,[sp,#16]
    ldp x9,x10,[sp]
    add sp,sp,#48
.global lab_held_blue_charge_return
lab_held_blue_charge_return:
    b .
.size lab_held_blue_charge_hook,.-lab_held_blue_charge_hook
.section .rodata
.balign 4
lab_held_blue_native_scale:
    .float 0.4
.global lab_held_blue_charge_scale
lab_held_blue_charge_scale:
    .word LAB_HELD_BLUE_SCALE_BITS
