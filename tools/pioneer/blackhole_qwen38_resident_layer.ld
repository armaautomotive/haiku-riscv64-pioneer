/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x72000 : { KEEP(*(.text.start)) . = ALIGN(4); }
 .conv 0x64000 : { *root_conv.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .matrix 0x68000 : { *matrix.o(.text .text.* .rodata .rodata.* .srodata .srodata.* .q38_*) . = ALIGN(4); }
 .norm 0x158000 : { *norm.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .ffn 0x15c000 : { *ffn.o(.text .text.* .rodata .rodata.* .srodata .srodata.* .q38_*) *gather.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .cohort 0xb8000 : { *(.text .text.* .rodata .rodata.* .srodata .srodata.* .q38_projection_data) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4 && cohort_entry==0x72000,"fixed new-layer frontend")
 ASSERT(SIZEOF(.text)<=0x2000,"frontend overlaps runtime")
 ASSERT(SIZEOF(.conv)>0 && SIZEOF(.conv)<=0x4000,"Conv overlaps matrix ROM")
 ASSERT(SIZEOF(.matrix)>0 && SIZEOF(.matrix)<=0x6000,"matrix overlaps stack")
 ASSERT(SIZEOF(.cohort)>0 && SIZEOF(.cohort)<=0x10000,"coordinator overlaps manifest")
 ASSERT(SIZEOF(.norm)>0 && SIZEOF(.norm)<=0x4000,"norm overlaps FFN ROM")
 ASSERT(SIZEOF(.ffn)>0 && SIZEOF(.ffn)<=0x22000,"FFN overlaps terminal region")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
