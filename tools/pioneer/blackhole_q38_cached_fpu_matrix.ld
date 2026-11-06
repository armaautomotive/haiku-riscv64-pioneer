/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"one-word harness boot")
 ASSERT(SIZEOF(.text)<=16384,"distinct service exceeds reserved16KiB before frame74000")
 ASSERT(cached_fpu_park<0x74000,"completion outside separate service ROM")
 ASSERT(cached_fpu_fault_park<0x74000,"fault outside separate service ROM")
}
