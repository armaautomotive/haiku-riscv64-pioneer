/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x40000 : { KEEP(*(.text.start)) *program.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .conv 0x64000 : { *root_conv.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4 && scalar_entry==0x40000,"exact NEW QRC1 entry")
 ASSERT(SIZEOF(.text)<=0x20000 && SIZEOF(.conv)<=0x8000,"ROOT and root Conv must not alias stack/worker state")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
