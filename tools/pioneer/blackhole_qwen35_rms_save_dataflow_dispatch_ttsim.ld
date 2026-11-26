/* SPDX-License-Identifier: MIT */
/* Simulator-only extra region: 0xd0000 lies in the old ROOT scratch range.
 * No driver may install this image without a separate saved-range authority. */
/* Non-loadable link witness. The resident worker does not invoke this region;
 * its graph authorization hook denies all commands. */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.flash 0xd0000 : {
        *service.o(.text._ZL15PioneerGraphFmafff .text._ZL26PioneerGraphDividePositiveff)
        *service.o(.text.*Flash* .rodata.*Flash* .srodata.*Flash*)
        *service.o(.text.*RmsSave* .rodata.*RmsSave* .srodata.*RmsSave*)
        *service.o(.text.qwen35_graph_flash_execute)
        *device-graph.o(.text.*Flash* .rodata.*Flash* .srodata.*Flash*)
        . = ALIGN(4);
    }
	.dispatch 0xa0400 : {
		/* Keep the normalization worker and its graph bridge in the already
		 * verified dispatch image, leaving room in the primary code region. */
		*service.o(.text.*PioneerQwen35DataflowRmsWorker*)
		/* Only present in the compile-only sharded recurrence witness. */
		*service.o(.text.*DataflowDeltaShardAssign*)
		*service.o(.text.*PioneerQwen35DataflowDeltaShardWorker*)
		/* The setup helper executes from the same saved, audited dispatch
		 * image; keeping it here leaves room for the resident matrix path. */
		*service.o(.text.*DataflowBlackholeTensixOps*TensixBeginOutput*)
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
	ASSERT(SIZEOF(.flash)>0 && SIZEOF(.flash)<=0x8000,"simulator flash code exceeds reserved region")
	ASSERT(SIZEOF(.boot)==4,"one fixed boot jump")
	ASSERT(qwen35_dataflow_entry==0x72000,"fixed dataflow service entry")
	ASSERT(SIZEOF(.text)<=0xe000,"primary code overlaps L1 queues")
	ASSERT(SIZEOF(.dispatch)>0 && SIZEOF(.dispatch)<=0x5c00,"dispatch code exceeds its L1 region")
	ASSERT(SIZEOF(.forbidden)==0,"service image cannot use writable globals")
}
