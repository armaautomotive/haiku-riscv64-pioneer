/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x72000 : { KEEP(*(.text.start)) *(.q38_main) . = ALIGN(4); }
 /* NEW graph owner MUST save/prove64000..6e000,NOT reuse the old16KiB
  * root-prefix lease. No native mapping/controller grants this footprint. */
 .graph 0x68000 : { *(.q38_graph) *gather.o(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .prefix 0x64000 : { *(.q38_* .text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"one fixed boot jump")
 ASSERT(block_entry==0x72000 && block_park==0x72038,"fixed graph frontend")
 ASSERT(block_fault_park==0x72060,"distinct graph fault entry")
 ASSERT(SIZEOF(.text)<=0x2000,"matrix body overlaps task metadata")
 ASSERT(SIZEOF(.prefix)<=0x4000,"matrix helpers overlap new graph")
 ASSERT(SIZEOF(.graph)>0 && SIZEOF(.graph)<=0x6000,"graph helpers overlap EXISTING8KiB stack")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
