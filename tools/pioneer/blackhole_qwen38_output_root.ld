/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	/* ROOT ONLY: already owned/saved immutable prefix; this role never uses
	 * the peer's70700 operand window. No wider view or descriptor overlap. */
	.output 0x70000 : {
		*(.q38_output .text._ZN24PioneerQ38OutputTransfer* .text._ZN12RootOutputIO*)
		*(.text._ZL10PartsValid* .text._ZL11HeaderValid*)
		. = ALIGN(4);
	}
	.text 0x72000 : {
		KEEP(*(.text.start)) *(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4);
	}
	.forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
	ASSERT(SIZEOF(.boot)==4,"one fixed boot jump")
	ASSERT(block_entry==0x72000,"fixed entry")
	ASSERT(SIZEOF(.output)<=0x2000,"root helper exceeds existing immutable prefix")
	ASSERT(SIZEOF(.text)<=0x2000,"root main overlaps descriptor")
	ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
