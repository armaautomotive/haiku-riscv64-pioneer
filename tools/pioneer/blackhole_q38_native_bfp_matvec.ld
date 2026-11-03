/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"new BFP reset jump must be one word")
 ASSERT(SIZEOF(.text)<=4096,"new BFP prototype exceeds its OWN4KiB ROM")
 ASSERT(native_bfp_park<0x71000,"new BFP park outside owned ROM")
 ASSERT(native_bfp_fault_park<0x71000,"new BFP fault park outside owned ROM")
}
