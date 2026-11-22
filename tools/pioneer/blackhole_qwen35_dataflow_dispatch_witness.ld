/* SPDX-License-Identifier: MIT */
/* Non-loadable link witness. The resident worker does not invoke this region;
 * its graph authorization hook denies all commands. */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.dispatch 0xa0400 : {
		/* Keep the normalization worker and its graph bridge in the already
		 * verified dispatch image, leaving room in the primary code region. */
		*service.o(.text.*PioneerQwen35DataflowRmsWorker*)
		*service.o(.text.qwen35_graph_execute)
		*device-stream.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		*device-barrier.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		*device-graph.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4);
	}
	.text 0x72000 : {
		KEEP(*(.text.start)) *(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4);
	}
	.forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
	ASSERT(SIZEOF(.boot)==4,"one fixed boot jump")
	ASSERT(qwen35_dataflow_entry==0x72000,"fixed dataflow service entry")
	ASSERT(SIZEOF(.text)<=0xe000,"primary code overlaps L1 queues")
	ASSERT(SIZEOF(.dispatch)>0 && SIZEOF(.dispatch)<=0x5c00,"dispatch code exceeds its L1 region")
	ASSERT(SIZEOF(.forbidden)==0,"service image cannot use writable globals")
}
