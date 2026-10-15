/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x70000 : { *(.text) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) }
	ASSERT(SIZEOF(.boot) == 4, "one-word TRISC0 jump required")
	ASSERT(SIZEOF(.text) == 0x1000, "fixed runtime code/data page required")
	ASSERT(drain < 0x70500, "main runtime code overlaps descriptor")
	ASSERT(runtime_descriptor == 0x70500, "fixed data-only descriptor required")
	ASSERT(resident_fetch == 0x70a40, "NoC code follows local unpack buffers")
	ASSERT(resident_fault_park < 0x71000, "NoC code exceeds code page")
}
