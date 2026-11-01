/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x64000 : {
  KEEP(*(.text.start)) *(.text .text.*)
  *(.rodata .rodata.* .srodata .srodata.*)
  . = ALIGN(4);
 }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"one fixed boot jump")
 ASSERT(dram_sweep_entry==0x64000,"distinct fixed offline entry")
 ASSERT(SIZEOF(.text)<=0xa000,"image cannot overlap 8KiB stack at 6e000")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
