/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x70000 : { *(.text) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) }
	ASSERT(SIZEOF(.boot) == 4, "TRISC0 reset jump must be one word")
	ASSERT(SIZEOF(.text) <= 2048, "fixed compute code footprint exceeded")
	ASSERT(parked < 0x70800, "park address outside code footprint")
}
