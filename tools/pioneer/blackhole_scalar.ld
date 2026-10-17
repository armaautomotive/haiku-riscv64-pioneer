/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x64000 : {
		KEEP(*(.text.start)) *(.text .text.*)
		*(.rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4); scalar_image_end = .;
	}
	.forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
	ASSERT(SIZEOF(.boot) == 4, "one fixed boot jump")
	ASSERT(scalar_entry == 0x64000, "fixed offline scalar entry")
	ASSERT(SIZEOF(.text) <= 0x8000, "scalar image exceeds offline32KiB code footprint")
	ASSERT(SIZEOF(.forbidden) == 0, "no static writable data/GOT")
}
