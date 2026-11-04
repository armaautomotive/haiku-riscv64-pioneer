/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"load-only boot must be one word")
 ASSERT(SIZEOF(.text)<=8192,"load-only image exceeds owned8KiB ROM")
 ASSERT(native_dram_load_park<0x72000,"load-only completion outside owned ROM")
 ASSERT(native_dram_load_fault_park<0x72000,"load-only fault outside owned ROM")
}
