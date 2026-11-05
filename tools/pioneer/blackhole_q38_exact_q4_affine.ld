/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"full-K affine reset jump is one word")
 ASSERT(SIZEOF(.text)<=8192,"new full-K affine exceeds its PRIVATE offboard8KiB ROM")
 ASSERT(exact_q4_affine_park<0x72000,"new affine park outside prototype ROM")
 ASSERT(exact_q4_affine_fault_park<0x72000,"new affine fault park outside prototype ROM")
}
