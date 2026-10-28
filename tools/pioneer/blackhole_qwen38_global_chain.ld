/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x40000 : { KEEP(*(.text.start)) *(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .terminal 0x60000 : { KEEP(*(.terminal)) }
 .conv 0x64000 : { *(.conv*) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4 && scalar_entry==0x40000,"exact DISTINCT global-chain entry")
 ASSERT(SIZEOF(.text)<=0x20000 && SIZEOF(.conv)<=0x8000,"unchanged ROOT/Conv ranges")
 ASSERT(SIZEOF(.terminal)==8 && scalar_fault_park==0x60000 && q38_global_chain_stopped==0x60004,"own global fault/STOP loops")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
