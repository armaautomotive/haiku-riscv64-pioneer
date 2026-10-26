/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x72000 : { KEEP(*(.text.start)) . = ALIGN(4); }
 .advance_matrix 0x40000 : { *advance_matrix.o(.text .text.* .rodata .rodata.* .srodata .srodata.* .q38_*) . = ALIGN(4); }
 .advance 0x46000 : { *advance.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .advance_gather 0x4e000 : { *advance_gather.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .continue 0x30000 : { *continue.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .conv 0x64000 : { *root_conv.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .matrix 0x68000 : { *matrix.o(.text .text.* .rodata .rodata.* .srodata .srodata.* .q38_*) . = ALIGN(4); }
 .norm 0x158000 : { *norm.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .link 0x159600 : { *link.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .ffn 0x15c000 : { *ffn.o(.text .text.* .rodata .rodata.* .srodata .srodata.* .q38_*) *gather.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .cohort 0xb8000 : { *(.text .text.* .rodata .rodata.* .srodata .srodata.* .q38_projection_data) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4 && cohort_entry==0x72000 && SIZEOF(.text)==128,"exact advance frontend")
 ASSERT(SIZEOF(.advance_matrix)>0 && SIZEOF(.advance_matrix)<=0x6000,"new matrix overlaps advance")
 ASSERT(SIZEOF(.advance)>0 && SIZEOF(.advance)<=0x8000,"advance overlaps gather")
 ASSERT(SIZEOF(.advance_gather)>0 && SIZEOF(.advance_gather)<=0x2000,"gather overlaps NEW data")
 ASSERT(SIZEOF(.continue)>0 && SIZEOF(.continue)<=0xa000,"continuation exceeds NEW lower-L1 ROM geometry")
 ASSERT(SIZEOF(.conv)==6472 && SIZEOF(.matrix)==14136 && SIZEOF(.cohort)==59288,"old layer components unchanged")
 ASSERT(SIZEOF(.norm)==5404 && SIZEOF(.link)==5976 && SIZEOF(.ffn)==30496,"old norm/link/FFN unchanged")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
