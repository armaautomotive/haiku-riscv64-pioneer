/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.text 0x70000 : { matrix.o(.text) }
	.bridge 0x71000 : { KEEP(*(.q38bridge)) }
	.unpack 0x72000 : {
		unpack.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4);
	}
	.forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
	ASSERT(SIZEOF(.boot)==4,"one fixed reset jump")
	ASSERT(SIZEOF(.text)==0x1000,"unchanged main code/data footprint")
	ASSERT(SIZEOF(.bridge)<=0x1000,"bridge exceeds fixed page")
	ASSERT(SIZEOF(.unpack)<=0x2000,"unpack overlaps fixed descriptor")
	ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
	ASSERT(drain<0x70500,"main code overlaps descriptor")
	ASSERT(runtime_descriptor==0x70500,"fixed matrix descriptor")
}
