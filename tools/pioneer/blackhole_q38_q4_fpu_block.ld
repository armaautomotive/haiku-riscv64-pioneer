/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"Q4 pipeline boot must be one word")
 ASSERT(SIZEOF(.text)<=8192,"Q4 pipeline exceeds its NEW private8KiB ROM")
 ASSERT(q4_fpu_block_park<0x72000,"completion outside private ROM")
 ASSERT(q4_fpu_block_fault_park<0x72000,"fault park outside private ROM")
}
