/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x70000 : { *(.text) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) }
	ASSERT(SIZEOF(.boot) == 4, "one-word TRISC0 reset jump required")
	ASSERT(SIZEOF(.text) == 0xa20, "fixed code/input footprint required")
	ASSERT(drain < 0x70700, "code overlaps private input buffers")
}
