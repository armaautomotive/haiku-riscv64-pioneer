/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x72000 : {
		KEEP(*(.text.start)) *start.o(.text .text.* .rodata .rodata.*)
		*program.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4); attention_image_end = .;
	}
	.rope 0x74020 : {
		KEEP(*(.rope_ext)) *rope.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4); attention_rope_end = .;
	}
	.forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
	ASSERT(SIZEOF(.boot) == 4, "one fixed boot jump")
	ASSERT(attention_entry == 0x72000, "fixed attention entry")
	ASSERT(SIZEOF(.text) <= 0x2000, "attention primary image exceeds owned8KiB")
	ASSERT(SIZEOF(.rope) <= 0x1fe0, "attention extension exceeds explicit separately backed-up gap")
	ASSERT(SIZEOF(.forbidden) == 0, "no static writable data / GOT")
}
