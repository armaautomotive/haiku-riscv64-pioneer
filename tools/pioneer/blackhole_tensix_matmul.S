/* SPDX-License-Identifier: MIT
 * Original fixed RV32I fixture for a sole, idle TRISC0 at Tensix (1,2).
 * Encodings/register facts audited against pinned official Blackhole ISA
 * and tt-llk 9ec671ce8922ce4cf26cc83e6ee1627cd8231c3b. No vendor code copied.
 * Requires explicit idle-coprocessor reset before launch. This image does
 * not preserve a previous compute context. No NoC/DMA/ARC commands exist.
 */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start:
	j main

.section .text,"ax"
.global main, parked
main:
	li s0, 0xffe40000 /* sole TRISC0 instruction FIFO */
	li s1, 0xffe80004 /* manual coprocessor completion, NOT a PCBuf pop */
	li s2, 0xffef0000 /* backend Config bank 0 */
	li s3, 0x17ffc0   /* fixed L1 status words */
	li t0, 1
	sw t0, 0(s3)
	fence
	/* Required first SETC16 selects configuration state 0. Then explicitly
	 * initialize all 68 thread-config entries. No .ttinsn fusion is used.
	 */
	li t0, 0xb2000000
	sw t0, 0(s0)
	jal ra, drain
	li t2, 1
	li t3, 68
thread_config:
	slli t4, t2, 16
	li t0, 0xb2000000
	or t0, t0, t4
	sw t0, 0(s0)
	addi t2, t2, 1
	bne t2, t3, thread_config
	li t0, 0xb2020001 /* disable implied SrcA format */
	sw t0, 0(s0)
	li t0, 0xb2030001 /* disable implied SrcB format */
	sw t0, 0(s0)
	jal ra, drain
	/* Explicit BF16 source formats, FP32 accumulators and IEEE FP32 RISC
	 * Dst mapping. Read each write back before issuing dependent work.
	 */
	li t0, 0x42b5
	sw t0, 0(s2)
	lw t1, 0(s2)
	addi zero, t1, 0
	li t0, 0x20aa0000
	sw t0, 4(s2)
	lw t1, 4(s2)
	addi zero, t1, 0
	sw zero, 8(s2)
	lw t1, 8(s2)
	addi zero, t1, 0
	sw zero, 12(s2)
	lw t1, 12(s2)
	addi zero, t1, 0
	sw zero, 24(s2)
	lw t1, 24(s2)
	addi zero, t1, 0
	sw zero, 880(s2) /* global Dst row remap/swizzle disabled */
	lw t1, 880(s2)
	addi zero, t1, 0
	fence
	li t0, 0x3700000f /* SETRWC: SrcA/SrcB/Dst/fidelity = 0 */
	sw t0, 0(s0)
	jal ra, drain
	li s4, 0
fixture:
	li t0, 0x3f800000 /* fixture 0: A=1, B=1 */
	li t1, 0x3f800000
	beq s4, zero, fill_inputs
	li t0, 0x40000000 /* fixture 1: A=2, B=3 */
	li t1, 0x40400000
fill_inputs:
	li t2, 0xffbd8000
	li t3, 256 /* A: sixteen rows, sixteen columns */
fill_a:
	sw t0, 0(t2)
	addi t2, t2, 4
	addi t3, t3, -1
	bne t3, zero, fill_a
	li t3, 128 /* B: eight rows, sixteen columns */
fill_b:
	sw t1, 0(t2)
	addi t2, t2, 4
	addi t3, t3, -1
	bne t3, zero, fill_b
	fence
	/* Only output rows are made undefined. Do not ZEROACC input rows:
	 * their validity was established by the prior explicit reset.
	 */
	li t0, 0x10000020
	li t2, 8
clear_output:
	sw t0, 0(s0)
	addi t0, t0, 1
	addi t2, t2, -1
	bne t2, zero, clear_output
	jal ra, drain
	/* Four rows per MOVD2A/B. No address increments or bank flips. */
	li t0, 0x08002000
	sw t0, 0(s0)
	li t0, 0x08082004
	sw t0, 0(s0)
	li t0, 0x08102008
	sw t0, 0(s0)
	li t0, 0x0818200c
	sw t0, 0(s0)
	li t0, 0x0a002010
	sw t0, 0(s0)
	li t0, 0x0a082014
	sw t0, 0(s0)
	jal ra, drain
	bne s4, zero, matrix
	/* Supported UNPACR_NOP handoffs, NOT unsupported SETDVALID. The
	 * implied format is disabled; both physical bank-0 operands become
	 * matrix-owned and remain so for the second fixture.
	 */
	li t0, 0x43000007
	sw t0, 0(s0)
	li t0, 0x43800007
	sw t0, 0(s0)
	jal ra, drain
matrix:
	li t0, 0x26000020 /* MVMUL Dst rows 32..39 += B[8,16] @ A[16,16] */
	sw t0, 0(s0)
	jal ra, drain
	/* Copy actual FPU results; the scalar core does no matrix arithmetic. */
	li t2, 0xffbd8800
	li t3, 0x17f800
	slli t4, s4, 9
	add t3, t3, t4
	li t5, 128
copy_output:
	lw t0, 0(t2)
	sw t0, 0(t3)
	addi t2, t2, 4
	addi t3, t3, 4
	addi t5, t5, -1
	bne t5, zero, copy_output
	fence
	addi s4, s4, 1
	sw s4, 4(s3)
	lw t0, 4(s3)
	addi zero, t0, 0
	fence
	li t0, 2
	bne s4, t0, fixture
	jal ra, drain
	li t0, 0x4d415431 /* MAT1 */
	sw t0, 8(s3)
	lw t1, 8(s3)
	addi zero, t1, 0
	fence
parked:
	j parked
drain:
	/* Publish the caller's return PC before waiting, so a timeout identifies
	 * which completion wait stalled without a debug-register write. */
	sw ra, 0(s3)
	lw t1, 0(s3)
	addi zero, t1, 0
	fence
	/* Ordered manual TTSync: do not omit the same-address store or the
	 * dependent ALU use. Fence flushes L0 and waits for this sequence.
	 */
	sw zero, 0(s1)
	lw t1, 0(s1)
	addi zero, t1, 0
	fence
	jalr zero, 0(ra)
