/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x72000 : { KEEP(*(.text.start)) . = ALIGN(4); }
 .conv 0x64000 : { *root_conv.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 /* Distinct NEW ROM only; no original native footprint grant. Capture ALL
  * special matrix sections before the coordinator's generic catch-all. */
 .matrix 0x68000 : { *matrix.o(.text .text.* .rodata .rodata.* .srodata .srodata.* .q38_*) . = ALIGN(4); }
 .cohort 0xb8000 : { *(.text .text.* .rodata .rodata.* .srodata .srodata.* .q38_projection_data) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4 && cohort_entry==0x72000,"fixed output coordinator frontend")
 ASSERT(SIZEOF(.text)<=0x2000,"coordinator overlaps runtime")
 ASSERT(SIZEOF(.conv)>0 && SIZEOF(.conv)<=0x4000,"root Conv overlaps matrix ROM")
 ASSERT(SIZEOF(.matrix)>0 && SIZEOF(.matrix)<=0x6000,"matrix ROM overlaps unchanged stack")
 ASSERT(SIZEOF(.cohort)>0 && SIZEOF(.cohort)<=0x10000,"coordinator ROM overlaps c8000 manifest")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
