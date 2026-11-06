/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"one-word comparison boot")
 ASSERT(SIZEOF(.text)<=16384,"comparison exceeds NEW private16KiB code scope")
 ASSERT(prepared_bf16_compare_park<0x74000,"comparison park outside code")
 ASSERT(prepared_bf16_compare_fault_park<0x74000,"comparison fault outside code")
}
