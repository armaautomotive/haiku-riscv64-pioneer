/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x72000 : { KEEP(*(.text.start)) *(.q38_main) . = ALIGN(4); }
 /* NEW private prefix,not the old70000 host-output helper view. Future
  * native controller MUST save/own/prove this ENTIRE phase-exclusive map. */
 .prefix 0x64000 : { *(.q38_* .text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4,"one fixed boot jump")
 ASSERT(block_entry==0x72000,"private FFN root entry")
 ASSERT(block_park==0x72038,"private FFN terminal park")
 ASSERT(block_fault_park==0x72060,"private FFN fault entry")
 ASSERT(SIZEOF(.text)<=0x2000,"FFN main overlaps task metadata")
 ASSERT(SIZEOF(.prefix)<=0x4000,"FFN helper overlaps reserved stack gap")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
