/* Shorten the ordinary random wait for local CPU single/triple Mushrooms.
 * This site is reached only after the native route-saving branch declines.
 * Preserve NZCV and every register except the displaced instruction's W1. */
.text
.balign 4
.global lab_mushroom_wait_hook
.type lab_mushroom_wait_hook,%function
lab_mushroom_wait_hook:
    sub sp,sp,#32
    stp x9,x10,[sp]
    stp x11,x12,[sp,#16]
    mrs x11,nzcv
    mov w1,#240
    ldr x9,[x19,#0x90]
    ldr x9,[x9,#0x88]
    ldr x9,[x9,#0x48]
    ldrb w10,[x9,#0xd2]
    cbz w10,.Lwait_done
    ldrb w10,[x9,#0xe7]
    cbnz w10,.Lwait_done
    ldr x9,[x19,#0xa8]
    ldr x9,[x9,#0x58]
    ldr w10,[x9,#0x9c]
    cmp w10,#3
    b.eq .Lmushroom_wait
    cmp w10,#7
    b.ne .Lwait_done
.Lmushroom_wait:
    mov w1,#90
.Lwait_done:
    msr nzcv,x11
    ldp x9,x10,[sp]
    ldp x11,x12,[sp,#16]
    add sp,sp,#32
.global lab_mushroom_wait_return
lab_mushroom_wait_return:
    b .
.size lab_mushroom_wait_hook,.-lab_mushroom_wait_hook
