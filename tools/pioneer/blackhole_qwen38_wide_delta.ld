/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x72000 : {
  KEEP(*(.text.start)) *(.text .text.*)
  *(.rodata .rodata.* .srodata .srodata.*)
  . = ALIGN(4); scalar_image_end = .;
 }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot) == 4, "one fixed local-only boot jump")
 ASSERT(scalar_entry == 0x72000, "fixed wide-delta local entry")
 ASSERT(scalar_park == 0x72038, "fixed wide-delta local park")
 ASSERT(SIZEOF(.text) <= 0x2000, "wide local code must not reach descriptor")
 ASSERT(SIZEOF(.forbidden) == 0, "no writable globals or GOT")
}
