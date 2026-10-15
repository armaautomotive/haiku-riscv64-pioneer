/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(pioneer_brisc_boot)
SECTIONS
{
	.boot 0 : { KEEP(*(.boot)) }
	.text 0x70000 : { *(.text) }
	/DISCARD/ : { *(.data) *(.bss) *(.comment) }
}
ASSERT(SIZEOF(.boot) == 4, "Boot image must touch only the reset word")
ASSERT(SIZEOF(.text) <= 128, "Private checksum image exceeds fixed code bound")
ASSERT((pioneer_brisc_start & 3) == 0, "Unaligned BRISC entry")
ASSERT(pioneer_brisc_start < 0x7ffff, "Outside audited startup-address bound")
