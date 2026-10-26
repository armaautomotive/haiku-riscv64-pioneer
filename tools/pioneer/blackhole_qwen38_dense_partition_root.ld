/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x50000 : {
  KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.* .srodata .srodata.*)
  . = ALIGN(4); partition_root_end = .;
 }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4 && scalar_entry==0x50000,"exact NEW standalone ROOT entry")
 ASSERT(SIZEOF(.text)<=0x8000,"new ROOT exceeds standalone32KiB;NOT a joint-layout grant")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
