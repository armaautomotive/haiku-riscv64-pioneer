/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x40000 : {
		KEEP(*(.text.start))
		*(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4);
	}
	.forbidden : { *(.data .data.* .bss .bss.* .sdata .sdata.* .sbss .sbss.* COMMON .got .got.*) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
	ASSERT(SIZEOF(.boot)==4,"one fixed boot jump")
	ASSERT(q35_work_entry==0x40000,"fixed ROOT entry")
	ASSERT(SIZEOF(.text)<=0x20000,"ROOT code and stack must remain disjoint")
	ASSERT(SIZEOF(.forbidden)==0,"ROOT must not depend on writable globals")
}
