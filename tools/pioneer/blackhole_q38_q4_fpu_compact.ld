/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"compact Q4 boot must be one word")
 ASSERT(SIZEOF(.text)<=8192,"compact Q4 exceeds ORIGINAL8KiB ROM bound")
 ASSERT(q4_fpu_compact_park<0x72000,"compact completion outside original ROM")
 ASSERT(q4_fpu_compact_fault_park<0x72000,"compact fault outside original ROM")
}
