/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(layer_return_test_begin_recurrent)
SECTIONS {
 .text 0x40000 : { KEEP(*(.text.layer_return_test_entry)) *(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .fault 0x60000 : { KEEP(*(.test_fault)) }
 .conv 0x64000 : { *(.conv*) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(layer_return_test_begin_recurrent==0x40000,"exact TEST caller entry")
 ASSERT(SIZEOF(.text)<=0x20000 && SIZEOF(.conv)<=0x8000,"unchanged ROOT/Conv ranges")
 ASSERT(SIZEOF(.fault)==8 && scalar_fault_park==0x60000 && layer_return_test_returned==0x60004,"explicit TEST fault/return loops")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
