/* CPU native steering uses +/-0.70; retail strong-charge compares >=0.71.
 * Only recognize CPU inward input here. Do not modify packet/angle/charge. */
.text
.balign 4
.global lab_cpu_inward_charge_hook
.type lab_cpu_inward_charge_hook,%function
lab_cpu_inward_charge_hook:
    sub sp,sp,#32
    stp x9,x10,[sp]
    str q3,[sp,#16]
    ldr x9,[x0,#8]
    cbz x9,.Lnative_charge
    ldrb w10,[x9,#0xd2]
    cbz w10,.Lnative_charge
    ldrb w10,[x9,#0xe7]
    cbnz w10,.Lnative_charge
    adrp x10,lab_cpu_inward_charge_threshold
    ldr s3,[x10,:lo12:lab_cpu_inward_charge_threshold]
    fcmp s0,s3
    b .Lcharge_done
.Lnative_charge:
    fcmp s0,s2
.Lcharge_done:
    ldp x9,x10,[sp]
    ldr q3,[sp,#16]
    add sp,sp,#32
.global lab_cpu_inward_charge_return
lab_cpu_inward_charge_return:
    b .
.size lab_cpu_inward_charge_hook,.-lab_cpu_inward_charge_hook
.section .rodata
.balign 4
.global lab_cpu_inward_charge_threshold
lab_cpu_inward_charge_threshold:
    .float 0.7
