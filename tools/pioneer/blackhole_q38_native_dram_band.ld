/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"DRAM band boot must be one word")
 ASSERT(SIZEOF(.text)<=8192,"DRAM band exceeds NEW private8KiB ROM")
 ASSERT(native_dram_band_park<0x72000,"DRAM band completion outside new owned ROM")
 ASSERT(native_dram_band_fault_park<0x72000,"DRAM band fault outside new owned ROM")
}
