/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x70000 : { *(.text) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) }
	ASSERT(SIZEOF(.boot) == 4, "one-word TRISC0 reset jump required")
	ASSERT(SIZEOF(.text) == 0x1000, "fixed resident code/data page required")
	ASSERT(drain < 0x70500, "matrix code overlaps input buffers")
	ASSERT(resident_fetch == 0x70a40, "resident fetch must follow all input buffers")
	ASSERT(resident_fault_park < 0x71000, "resident fetch exceeds code page")
}
