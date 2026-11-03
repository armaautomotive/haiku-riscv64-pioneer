/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"native band boot must be one word")
 ASSERT(SIZEOF(.text)<=4096,"native band exceeds private4KiB ROM")
 ASSERT(native_bfp_band_park<0x71000,"native band completion outside owned ROM")
 ASSERT(native_bfp_band_fault_park<0x71000,"native band fault outside owned ROM")
}
