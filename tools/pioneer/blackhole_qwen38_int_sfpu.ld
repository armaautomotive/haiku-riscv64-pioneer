/* SPDX-License-Identifier: MIT */
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
	.boot 0x6000 : { KEEP(*(.boot)) }
	/* Already-owned immutable prefix. NEVER overlap live operands70700..70a20,
	 * stack6e000..70000, original main text72000..74000 or descriptor74000. */
	/* Pin ONLY the integer specialization, including the compiler's local
	 * constant-propagation clone suffix. A changed symbol fails nonempty. */
	.affine 0x70b00 : {
		/* Swap ONLY cold once/job helpers for the offline replay variant. */
		*(.text.pioneer_q38_replay_job_setup)
		*(.text._Z24PioneerQ38BlockAffineDstI18VectorPanelEmitterE*)
		*(.text._Z23PioneerQ38VectorDstInitI18VectorPanelEmitterEvRT_bbbb*)
		*(.text._Z25PioneerQ38SfpuAccumulatorI18VectorPanelEmitterLb1EE*)
		/* Separately gated zero-minimum variant: preserve original arithmetic
		 * as a distinct constant specialization for Q4/ALL checked jobs. */
		*(.text._Z25PioneerQ38SfpuAccumulatorI18VectorPanelEmitterLb1ELb0EE*)
		*(.text._Z25PioneerQ38SfpuAccumulatorI18VectorPanelEmitterLb1ELb1EE*)
		*(.text._Z25PioneerQ38SfpuAccumulatorI18VectorPanelEmitterLb1ELb0ELb0EE*)
		*(.text._Z25PioneerQ38SfpuAccumulatorI18VectorPanelEmitterLb1ELb0ELb1EE*)
		*(.text._Z25PioneerQ38SfpuAccumulatorI18VectorPanelEmitterLb1ELb1ELb1EE*)
		/* Separate block-metadata specializations;SAME reserved prefix. */
		*(.text._Z25PioneerQ38SfpuAccumulatorI18VectorPanelEmitterLb1ELb0ELb0ELb0EE*)
		*(.text._Z25PioneerQ38SfpuAccumulatorI18VectorPanelEmitterLb1ELb0ELb1ELb0EE*)
		*(.text._Z25PioneerQ38SfpuAccumulatorI18VectorPanelEmitterLb1ELb1ELb1ELb0EE*)
		*(.text._Z25PioneerQ38SfpuAccumulatorI18VectorPanelEmitterLb1ELb0ELb1ELb1EE*)
		*(.text._ZN22PioneerQ38CachedAffine*)
		/* Separate OFFLINE direct-Q4 helper,SAME owned prefix/limits. */
		*(.text.pioneer_q38_direct_q4)
		*(.text._Z24PioneerQ38DirectQ4UnpackI18VectorPanelEmitterE*)
		/* OFFLINE sticky-Q4 helpers;SAME fixed prefix/limits. */
		*(.text._Z23PioneerQ38DirectQ4SetupI18VectorPanelEmitterE*)
		/* New SM7 Q8 job setup,SAME prefix/authority;absent by default. */
		*(.text._Z29PioneerQ38DirectNativeQ8SetupI18VectorPanelEmitterE*)
		/* Unselected prepared-Q4 experiment,inside the SAME owned prefix. */
		*(.text._ZN20PioneerQ38PreparedQ45Panel*)
		*(.text._ZN20PioneerQ38PreparedQ45Block*)
		. = ALIGN(4);
	}
	.text 0x72000 : {
		KEEP(*(.text.start)) *(.text .text.* .rodata .rodata.* .srodata .srodata.*)
		. = ALIGN(4);
	}
	.forbidden : { *(.data .data.* .sdata .sdata.* .bss .bss.* .sbss .sbss.* COMMON .got .got.*) }
	/DISCARD/ : { *(.riscv.attributes) *(.comment) *(.eh_frame*) }
	ASSERT(SIZEOF(.boot)==4,"one fixed boot jump")
	ASSERT(block_entry==0x72000,"fixed entry")
	ASSERT(SIZEOF(.affine)>0 && SIZEOF(.affine)<=0x1500,"affine helpers must fit already-owned70b00..72000")
	ASSERT(SIZEOF(.text)<=0x2000,"main image overlaps descriptor")
	ASSERT(SIZEOF(.forbidden)==0,"no writable globals/GOT")
}
