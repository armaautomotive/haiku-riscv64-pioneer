/* SPDX-License-Identifier: MIT */
/* Inspection link only. No native ownership, upload or hardware authority. */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
 .bf16 0x20000 : { *(.bf16*) *(.text._ZN31PioneerQ38DenseQ8MatrixRootJobs5Build*) *(.text._ZN35PioneerQ38RecurrentQ8MatrixRootJobs5Build*) *(.text._ZN28PioneerQ38HybridWorkerLayoutL9LeafShapeI23PioneerQ38Bfp4WorkerJobEEbPKjS3_*) . = ALIGN(4); }
 ASSERT(SIZEOF(.bf16)>0 && SIZEOF(.bf16)<=0x10000,"BF16 coordinator range")
 .boot 0x6000 : { KEEP(*(.boot)) }
 .text 0x40000 : { KEEP(*(.text.start)) *(.text .text.* .rodata .rodata.* .srodata .srodata.*) . = ALIGN(4); }
 .terminal 0x60000 : { KEEP(*(.terminal)) }
 .conv 0x64000 : { *(.conv*) . = ALIGN(4); }
 .forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
 /DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
 ASSERT(SIZEOF(.boot)==4 && scalar_entry==0x40000,"distinct global-chain entry")
 ASSERT(SIZEOF(.text)<=0x20000 && SIZEOF(.conv)<=0x8000,"ROOT and Conv ranges")
 ASSERT(SIZEOF(.terminal)==8 && scalar_fault_park==0x60000 && q38_global_chain_stopped==0x60004,"fault/STOP loops")
 ASSERT(SIZEOF(.forbidden)==0,"no writable globals or GOT")
}
