/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x72000 : {
		KEEP(*(.text.start)) *(.text .text.*)
		*(.rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4);
		attention_image_end = .;
	}
	.forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
	ASSERT(SIZEOF(.boot) == 4, "one fixed boot jump")
	ASSERT(attention_entry == 0x72000, "fixed attention entry")
	ASSERT(SIZEOF(.text) <= 0x2000, "attention image exceeds separately owned 8 KiB")
	ASSERT(SIZEOF(.forbidden) == 0, "no static writable data / GOT")
}
