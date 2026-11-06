/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"one-word factored harness boot")
 ASSERT(SIZEOF(.text)<=16384,"factored service exceeds reserved16KiB before frame74000")
 ASSERT(factored_q4_park<0x74000,"completion outside factored service ROM")
 ASSERT(factored_q4_fault_park<0x74000,"fault outside factored service ROM")
}
