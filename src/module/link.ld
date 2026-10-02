OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)
ENTRY(lab_module_start)
PHDRS {
  text PT_LOAD FLAGS(5);
  rodata PT_LOAD FLAGS(4);
  data PT_LOAD FLAGS(6);
  dynamic PT_DYNAMIC FLAGS(6);
}
SECTIONS {
  . = 0;
  .text : { KEEP(*(.text.header)) *(.text .text.*) } :text
  . = ALIGN(0x1000);
  .rodata : { KEEP(*(.rodata.mod0)) *(.rodata .rodata.*) } :rodata
  .hash : { *(.hash) } :rodata
  .dynsym : { *(.dynsym) } :rodata
  .dynstr : { *(.dynstr) } :rodata
  .rela.dyn : { *(.rela.dyn) } :rodata
  . = ALIGN(0x1000);
  .data : { *(.data .data.*) } :data
  .got : { *(.got .got.*) } :data
  .dynamic : { HIDDEN(lab_dynamic = .); *(.dynamic) } :data :dynamic
  .bss (NOLOAD) : {
    . = ALIGN(16);
    HIDDEN(lab_bss_start = .);
    *(.bss .bss.*) *(COMMON)
    . = ALIGN(16);
    HIDDEN(lab_bss_end = .);
  } :data
  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
