/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x70000 : { KEEP(*(.text.start)) *(.text .text.*) *(.rodata .rodata.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"one-word trace boot")
 ASSERT(SIZEOF(.text)<=16384,"trace exceeds NEW private16KiB code scope")
 ASSERT(factored_trace_park<0x74000,"trace park outside code")
 ASSERT(factored_trace_fault_park<0x74000,"trace fault outside code")
}
