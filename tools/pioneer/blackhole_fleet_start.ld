/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(fleet_start)
SECTIONS {
	.text 0x71000 : { *(.text) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) }
	ASSERT(fleet_start == 0x71000, "fixed fleet prelude entry")
	ASSERT(SIZEOF(.text) <= 0x1000, "fleet prelude must fit separately owned page")
}
