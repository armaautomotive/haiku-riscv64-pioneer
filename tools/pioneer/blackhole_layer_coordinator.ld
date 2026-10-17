/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x64000 : {
		KEEP(*(.text.start)) *(.text .text.*)
		*(.rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4); coordinator_image_end = .;
	}
	.forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
	ASSERT(SIZEOF(.boot) == 4, "one fixed layer boot jump")
	ASSERT(coordinator_entry == 0x64000, "fixed separate layer entry")
	ASSERT(SIZEOF(.text) <= 0x8000, "layer image exceeds separate32KiB code prerequisite")
	ASSERT(SIZEOF(.forbidden) == 0, "no static writable data/GOT")
}
