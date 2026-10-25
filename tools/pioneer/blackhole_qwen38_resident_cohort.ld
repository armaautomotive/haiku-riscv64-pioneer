/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x72000 : { KEEP(*(.text.start)) . = ALIGN(4); }
 /* ROOT0 computes its SIX groups with the SAME precise QCB1 arithmetic. */
 .conv 0x64000 : { *root_conv.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 /* NEW explicit schedule ROM+manifest/shadow footprint,NO old native grant. */
 .cohort 0xb8000 : { *(.text .text.* .rodata .rodata.* .srodata .srodata.* .q38_projection_data) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4 && cohort_entry==0x72000,"fixed new cohort frontend")
 ASSERT(SIZEOF(.text)<=0x2000,"cohort frontend overlaps runtime descriptors")
 ASSERT(SIZEOF(.conv)>0 && SIZEOF(.conv)<=0x4000,"ROOT0 Conv overlaps unchanged stack")
 ASSERT(SIZEOF(.cohort)>0 && SIZEOF(.cohort)<=0x10000,"ROOT schedule overlaps NEW c8000 manifest")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
