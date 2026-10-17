/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x6c000 : {
		KEEP(*(.text.start)) *(.text .text.*)
		*(.rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4); coordinator_image_end = .;
	}
	.forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
	ASSERT(SIZEOF(.boot) == 4, "one fixed boot jump")
	ASSERT(coordinator_entry == 0x6c000, "fixed coordinator entry")
	ASSERT(SIZEOF(.text) <= 0x2000, "coordinator exceeds separate 8KiB code lease")
	ASSERT(SIZEOF(.forbidden) == 0, "no static writable data/GOT")
}
