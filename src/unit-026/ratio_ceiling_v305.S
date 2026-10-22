/* Replace only the CPU publisher's two upper-bound constants. Each hook
 * writes exactly the original instruction's s2 output; integer registers,
 * stack and NZCV stay untouched. The native floor, formation and catch math
 * still execute. +Infinity represents the explicit no-ceiling policy. */
.text
.balign 4
.global lab_cpu_ratio_fallback_hook
.type lab_cpu_ratio_fallback_hook,%function
lab_cpu_ratio_fallback_hook:
    ldr s2, lab_cpu_ratio_ceiling_value
.global lab_cpu_ratio_fallback_return
lab_cpu_ratio_fallback_return:
    b .
.size lab_cpu_ratio_fallback_hook,.-lab_cpu_ratio_fallback_hook

.global lab_cpu_ratio_formation_hook
.type lab_cpu_ratio_formation_hook,%function
lab_cpu_ratio_formation_hook:
    ldr s2, lab_cpu_ratio_ceiling_value
.global lab_cpu_ratio_formation_return
lab_cpu_ratio_formation_return:
    b .
.size lab_cpu_ratio_formation_hook,.-lab_cpu_ratio_formation_hook

.section .rodata.lab_cpu_ratio_ceiling,"a",@progbits
.balign 4
.global lab_cpu_ratio_ceiling_value
.type lab_cpu_ratio_ceiling_value,%object
lab_cpu_ratio_ceiling_value:
    .word LAB_CPU_RATIO_CEILING_BITS
.size lab_cpu_ratio_ceiling_value,4
