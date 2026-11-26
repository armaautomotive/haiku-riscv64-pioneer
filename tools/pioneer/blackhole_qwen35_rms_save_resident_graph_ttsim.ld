/* SPDX-License-Identifier: MIT */
/* Simulator-only extra region: 0xd0000 lies in the old ROOT scratch range.
 * No driver may install this image without a separate saved-range authority. */
/* Offline resident graph link witness. No loadable driver invokes this image. */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	.flash 0xd0000 : {
        *service.o(.text._ZL15PioneerGraphFmafff .text._ZL26PioneerGraphDividePositiveff)
        *service.o(.text._ZL22PioneerAttentionToHalff .text._ZL24PioneerGraphSqrtPositivef .text._ZL15PioneerGraphExpf)
        *service.o(.text.*Flash* .rodata.*Flash* .srodata.*Flash*)
        *service.o(.text.*RmsSave* .rodata.*RmsSave* .srodata.*RmsSave*)
        *service.o(.text.qwen35_graph_flash_execute)
        *device-graph.o(.text.*Flash* .rodata.*Flash* .srodata.*Flash*)
        . = ALIGN(4);
    }
	.dispatch 0xa0400 : {
		/* Shared arithmetic helpers execute from the saved dispatch region,
		 * leaving space in the primary image for the resident graph. */
		*service.o(.text._ZL15PioneerGraphFmafff)
		*service.o(.text._ZL26PioneerGraphDividePositiveff)
		/* Keep the normalization worker and its graph bridge in the already
		 * verified dispatch image, leaving room in the primary code region. */
		*service.o(.text.*PioneerQwen35DataflowRmsWorker*)
		/* The optional sharded recurrence uses this small fixed assignment
		 * helper; placing it here keeps the primary image below its L1 limit. */
		*service.o(.text.*DataflowDeltaShardAssign*)
		*service.o(.text.*PioneerQwen35DataflowDeltaShardWorker*)
		/* The setup helper executes from the same saved, audited dispatch
		 * image; keeping it here leaves room for the resident matrix path. */
		*service.o(.text.*DataflowBlackholeTensixOps*TensixBeginOutput*)
		/* Keep invocation validation with the saved dispatch image as the
		 * experimental sharing metadata adds checks to the primary path. */
		*service.o(.text._ZNK13PioneerQwen3522DataflowCoreInvocation5ValidEv)
		/* The shared compute image owns this matrix helper. Its range is
		 * saved and installed with the rest of the dispatch image. */
		*shared-roles.o(.text.*DataflowBlackholeTensixOps*MatrixPanel*)
		*service.o(.text.qwen35_graph_execute)
		*device-stream.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		*device-barrier.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		*device-graph.o(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		/* Place the compact resident control path beside the existing graph
		 * dispatcher; the saved primary code range must not reach the queues. */
		*resident-service-entry.o(.text.*DataflowResidentLifecycle*)
		*resident-service-entry.o(.text.*DataflowResidentServiceRun*)
		*resident-service-entry.o(.text.*DataflowResidentCommand*)
		*resident-service-entry.o(.text.*DataflowResidentFanout*)
		*resident-service-entry.o(.text.*DataflowResidentCandidate*)
		*resident-service-entry.o(.text.*DataflowRoleMailboxIdle*)
		*resident-service-entry.o(.text.*DataflowResidentQueuesClean*)
		*resident-service-entry.o(.text.*DataflowResidentWorkerIO*ResidentResetAndReady*)
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
	/* 0xa0400 + 0x7000 = 0xa7400, below the 0xaa000 invocation. */
	ASSERT(SIZEOF(.dispatch)>0 && SIZEOF(.dispatch)<=0x7000,"dispatch code exceeds its L1 region")
	ASSERT(SIZEOF(.forbidden)==0,"service image cannot use writable globals")
}
