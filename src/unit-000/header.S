// Minimal NSO module header. RTLD invokes DT_INIT after relocation.
// Format reference: https://switchbrew.org/wiki/NSO and exlaunch documentation.
.section .text.header,"ax"
.global lab_module_start
.hidden lab_module_start
lab_module_start:
    ret
    .word lab_mod0 - lab_module_start

.section .rodata.mod0,"a"
.balign 4
lab_mod0:
    .ascii "MOD0"
    .word lab_dynamic - lab_mod0
    .word lab_bss_start - lab_mod0
    .word lab_bss_end - lab_mod0
    .word 0
    .word 0
    .word lab_runtime_object - lab_mod0

.section .bss.runtime,"aw",%nobits
.balign 16
.global lab_runtime_object
.hidden lab_runtime_object
lab_runtime_object:
    .skip 0x100
