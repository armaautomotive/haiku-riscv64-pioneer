/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j block_entry
.section .text.start,"ax"
.global block_entry,block_done,block_park,block_fault_park
block_entry:
	li sp,0x70000
	call block_main
block_done:
	li t0,0x17ffc0
	sw a0,4(t0)
	la t1,block_done
	sw t1,0(t0)
.ifdef TT_Q38_DISPATCH_ROOT
.ifdef TT_Q38_PROJECTION_GRAPH_ROOT
.ifdef TT_Q38_GDN_OUTPUT_GRAPH_ROOT
	li t1,0x31504f51 /* QOP1:distinct gated-input complete ssm_out */
.else
.ifdef TT_Q38_PROJECTION_RESULT_STASH
	li t1,0x31535051 /* QPS1:separate fixed resident projection results */
.else
	li t1,0x31525051 /* QPR1:NEW complete resident-input projection */
.endif
.endif
.else
.ifdef TT_Q38_FFN_GRAPH_ROOT
.ifdef TT_Q38_FFN_GRAPH_RESIDUAL_ROOT
	li t1,0x33474651 /* QFG3:card-resident input+preserved residual,not QFG1/2 */
.else
.ifdef TT_Q38_FFN_GRAPH_INPUT_ROOT
	li t1,0x32474651 /* QFG2:owned initial F32 producer,NOT QFG1 */
.else
	li t1,0x31474651 /* QFG1:NEW complete FFN root,not an old projection */
.endif
.endif
.else
.ifdef TT_Q38_FFN_MATRIX_ROOT
.ifdef TT_Q38_PROJECTION_MATRIX_ROOT
	li t1,0x314d5051 /* QPM1:NEW generic projection wave,not QFM1 */
.else
	li t1,0x314d4651 /* QFM1:NEW private resident FFN root ONLY */
.endif
.else
	li t1,0x314d3851 /* Q8M1:OFFLINE persistent-dispatch root ONLY */
.endif
.endif
.endif
.else
.ifdef TT_Q38_VECTOR_ROWS
	li t1,0x31563851 /* Q8V1:OFFLINE vector-unpack rows ONLY */
.else
.ifdef TT_Q38_SHARED_ROWS
	li t1,0x31483851 /* Q8H1:shared root inputs,NOT driver50 */
.else
.ifdef TT_Q38_ROWS
	li t1,0x31523851 /* Q8R1:OFFLINE fullK,multiple output row groups */
.else
.ifdef TT_Q38_STREAM
	li t1,0x31533851 /* Q8S1:OFFLINE fullK stream,not local block48 */
.else
	li t1,0x31423851 /* Q8B1:distinct from one-panel completion */
.endif
.endif
.endif
.endif
.endif
	sw t1,8(t0)
	lw t1,8(t0)
	fence
block_park: j block_park
.ifdef TT_Q38_DISPATCH_ROOT
	/* Both the architectural branch and speculative next fetch are SAFE.
	 * Never make the accepted next address alias the fault entry. */
	j block_park
.org 0x60
.endif
block_fault_park:
	li t0,0x17ffc8
.ifdef TT_Q38_DISPATCH_ROOT
.ifdef TT_Q38_PROJECTION_GRAPH_ROOT
.ifdef TT_Q38_GDN_OUTPUT_GRAPH_ROOT
	li t1,0x46504f51 /* QOPF:unknown, no later IO or cleanup */
.else
.ifdef TT_Q38_PROJECTION_RESULT_STASH
	li t1,0x46535051 /* QPSF:UNKNOWN,NO later IO/cleanup */
.else
	li t1,0x46525051 /* QPRF:UNKNOWN,no later IO/cleanup */
.endif
.endif
.else
.ifdef TT_Q38_FFN_GRAPH_ROOT
	li t1,0x46474651 /* QFGF:terminal UNKNOWN,NO cleanup authority */
.else
.ifdef TT_Q38_FFN_MATRIX_ROOT
.ifdef TT_Q38_PROJECTION_MATRIX_ROOT
	li t1,0x464d5051 /* QPMF:UNKNOWN,no later IO/cleanup */
.else
	li t1,0x464d4651 /* QFMF:UNKNOWN,NO later card IO/cleanup */
.endif
.else
	li t1,0x464d3851 /* Q8MF:UNKNOWN,no later packet/reset/restore */
.endif
.endif
.endif
.else
.ifdef TT_Q38_VECTOR_ROWS
	li t1,0x46563851 /* Q8VF:UNKNOWN,NO further packet/reset/restore */
.else
.ifdef TT_Q38_SHARED_ROWS
	li t1,0x46483851 /* Q8HF:NO retry/reset/restore after UNKNOWN */
.else
.ifdef TT_Q38_ROWS
	li t1,0x46523851 /* Q8RF:NO retry/reset/restore after uncertainty */
.else
.ifdef TT_Q38_STREAM
	li t1,0x46533851 /* Q8SF:NO retry/reset/restore after uncertain fetch */
.else
	li t1,0x46423851 /* Q8BF:NO success/stop permission */
.endif
.endif
.endif
.endif
.endif
	sw t1,0(t0)
	lw t1,0(t0)
	fence
1: j 1b
