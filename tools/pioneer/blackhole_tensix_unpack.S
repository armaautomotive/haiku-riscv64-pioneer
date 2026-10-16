/* SPDX-License-Identifier: MIT
 * Original bounded TRISC0 L1-unpack fixture. Register/encoding facts from
 * pinned tt-lab e6baf562a72491ed9d594cddbdf42e0925b1f79e and public LLK.
 * Default variants issue no NoC requests. The separate TT_UNPACK_RESIDENT
 * variant performs two fixed GDDR reads. No host DMA, ARC messages, packer
 * or firmware changes in any variant.
 */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start:
	j main

.macro emit value
	li t0, \value
	sw t0, 0(s0)
.endm
.macro config index, value
	li t0, \value
	sw t0, (4*\index)(s2)
	lw t1, (4*\index)(s2)
	andi t1, t1, 0
.endm

.section .text,"ax"
.global main, parked, final_wait_return, drain
.ifdef TT_RUNTIME_Q8
	.set TT_RUNTIME_INT8,1
.endif
.ifdef TT_RUNTIME_INT8
	.set TT_RUNTIME_FP16,1 /* integer-8 uses the FP16 source bit overlay */
.endif
main:
	li s0, 0xffe40000
	li s1, 0xffe000fc /* ThCon GPR63, shared with this TRISC0 */
	li s2, 0xffef0000
	li s3, 0x17ffc0
	li s4, 0
.ifdef TT_UNPACK_GEMV
.ifdef TT_RUNTIME_MATVEC
	li t0, 0x70500 /* trusted data-only descriptor: weight base, K/16, N/16 */
	lw a0, 0(t0)
	lw s8, 4(t0)
	lw s11, 8(t0)
	li a1, 0
	li s9, 0
	li s10, 0x84000
.else
	.global gemv_count_instruction
gemv_count_instruction:
.ifndef TT_GEMV_INPUTS
	.set TT_GEMV_INPUTS, 1024
.endif
.if TT_GEMV_INPUTS != 1024 && TT_GEMV_INPUTS != 5120 && TT_GEMV_INPUTS != 6144
	.error "unsupported fixed GEMV shape"
.endif
	li s8, (TT_GEMV_INPUTS/16)
.endif
.endif
	emit 0xb2000000 /* first select configuration state zero */
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
	emit 0xb2020001 /* explicit source formats, no implied-format reliance */
	emit 0xb2030001
	emit 0xb2050004 /* SrcA address override, as in normal unpack setup */
	jal ra, drain
.ifdef TT_RUNTIME_FP16
.ifndef TT_RUNTIME_MATVEC
	.error "FP16 candidate is restricted to the fixed runtime matvec"
.endif
	config 0, 0x4231 /* explicit FP16 SrcA/SrcB, FP32 Dst override */
.ifdef TT_RUNTIME_INT8
	config 1, 0xa0220000 /* force integer matrix arithmetic, not FP16 math */
.else
	config 1, 0x20220000 /* FP16 source fields, FP32 accumulation */
.endif
.else
	config 0, 0x42b5
	config 1, 0x20aa0000
.endif
	config 2, 0
.ifdef TT_RUNTIME_INT8
.ifdef TT_RUNTIME_Q8
	config 3, 0 /* FP32 scales/results; SFPU consumes sign-magnitude int dots */
.else
	config 3, 0x10000 /* converted signed int32 RISC destination read */
.endif
.else
	config 3, 0 /* TRISC0 RISC Dst access = converted IEEE FP32 */
.endif
	config 6, 0
	config 7, 0
	config 220, 0
	/* Fixed uncompressed BF16 descriptors, source bases and local strides.
	 * A: 16x16 = 512 bytes at 70710. B: 16x8 = 256 bytes at 70920.
	 * Each has a private 16-byte header immediately before its payload.
	 */
	config 20, 0
	config 24, 0xffff
	config 28, 0x100
	config 50, 0x100
	config 56, 0x00100000
	config 59, 0x100
.ifdef TT_RUNTIME_FP16
	config 64, 0x00100011
.else
	config 64, 0x00100015
.endif
	config 65, 16
.ifdef TT_RUNTIME_FP16
	config 72, 1
.else
	config 72, 5
.endif
	config 73, 3
	config 76, 0x7070 /* payload/16 - 1 */
	config 84, 0x00400040
	config 86, 0x00100010
.ifdef TT_RUNTIME_FP16
	config 112, 0x00100011
.else
	config 112, 0x00100015
.endif
	config 113, 8
.ifdef TT_RUNTIME_FP16
	config 120, 1
.else
	config 120, 5
.endif
	config 121, 3
	config 124, 0x7091
	fence
	emit 0x5e23fc00 /* unpack0 X bounds 0..255 BF16 elements */
	emit 0x5e41fc00 /* unpack1 X bounds 0..127 BF16 elements */
	jal ra, drain
.ifdef TT_RUNTIME_MATVEC
	/* Only row zero is a vector. Weight fetch padding overwrites that
	 * row, not the other seven rows, which remain zero for all K/N tiles. */
	li t2, 0x70920
	li t1, 64
runtime_zero_b:
	sw zero, 0(t2)
	addi t2, t2, 4
	addi t1, t1, -1
	bne t1, zero, runtime_zero_b
.endif
fixture:
.ifdef TT_UNPACK_CONFIG
	j finish_fixture /* control only: no input writes, unpack or matrix */
	nop /* preserve the two-word li width and common completion PCs */
.else
	li t2, 0x70700
.endif
	sw zero, 0(t2)
	sw zero, 4(t2)
	sw zero, 8(t2)
	sw zero, 12(t2)
	li t2, 0x70910
	sw zero, 0(t2)
	sw zero, 4(t2)
	sw zero, 8(t2)
	sw zero, 12(t2)
.ifdef TT_UNPACK_NONUNIFORM
.ifdef TT_UNPACK_RESIDENT
	j resident_fetch
.else
	j nonuniform_inputs
.endif
	nop /* same width as li, preserve every common code address */
.else
	li t0, 0x3f803f80
.endif
	li t3, 0x3f803f80
	beq s4, zero, inputs
	li t0, 0x40004000
	li t3, 0x40404040
inputs:
	li t2, 0x70710
	li t1, 128
fill_a:
	sw t0, 0(t2)
	addi t2, t2, 4
	addi t1, t1, -1
	bne t1, zero, fill_a
	li t2, 0x70920
	li t1, 64
fill_b:
	sw t3, 0(t2)
	addi t2, t2, 4
	addi t1, t1, -1
	bne t1, zero, fill_b
inputs_ready:
	lw t1, -4(t2) /* dependent readback before issuing an L1 consumer */
	andi t1, t1, 0
	fence
	emit 0x3700000f /* reset all RWCs, no ownership side effects */
.ifdef TT_UNPACK_GEMV
.ifdef TT_RUNTIME_Q8
	j q8_clear /* clear the integer dot at every 32-value block boundary */
.else
	bne s4, zero, gemv_keep_accumulator
.endif
.endif
clear_first:
	li t0, 0x10180000 /* clear all Dst valid bits; no Dst input is in use */
	nop /* same width as the former two-word one-row clear immediate */
	li t2, 1
clear_output:
	sw t0, 0(s0)
	addi t0, t0, 1
	addi t2, t2, -1
	bne t2, zero, clear_output
.ifdef TT_UNPACK_GEMV
gemv_keep_accumulator:
.endif
	jal ra, drain
	emit 0x5160000a /* reset Y only; preserve X0 start and X1 end bounds */
	emit 0x5460000f /* zero Z/W ADCs for both unpackers/channels */
	emit 0x420000c1 /* normal unpack0, explicit thread, set valid, last */
	emit 0x428000c1 /* normal unpack1, explicit thread, set valid, last */
	jal ra, drain
.ifdef TT_UNPACK_SRCA
	emit 0x12002020 /* SrcA eight rows -> Dst32; no multiplication */
.else
.ifdef TT_UNPACK_SRCB
	emit 0x13001020 /* SrcB row0 broadcast -> Dst32; no multiplication */
.else
.ifdef TT_UNPACK_FULL_BF16
	j full_bf16_math
	nop
	nop /* preserve the common three-word emit and completion addresses */
.else
	emit 0x26c00020 /* MVMUL, release/flip both operand banks on completion */
.endif
.endif
.endif
	jal ra, drain
full_bf16_return:
.ifdef TT_RUNTIME_Q8
	j q8_accumulate
q8_accumulate_return:
.endif
.ifdef TT_UNPACK_GEMV
	addi s4, s4, 1
.ifdef TT_RUNTIME_MATVEC
	addi a1, a1, 1
	sw a1, 4(s3)
.else
	sw s4, 4(s3)
.endif
	lw t0, 4(s3)
	andi t0, t0, 0
	bne s4, s8, fixture /* all K tiles accumulate on-card, no host partial sums */
.endif
	li t2, 0xffbd8800
.ifdef TT_RUNTIME_MATVEC
	addi t3, s10, 0
.else
	li t3, 0x17f800
.endif
.ifndef TT_UNPACK_GEMV
	slli t4, s4, 9
	add t3, t3, t4
.endif
.ifdef TT_RUNTIME_MATVEC
	li t5, 16
.else
	li t5, 128
.endif
copy_output:
	lw t0, 0(t2)
	sw t0, 0(t3)
	addi t2, t2, 4
	addi t3, t3, 4
	addi t5, t5, -1
	bne t5, zero, copy_output
	fence
.ifdef TT_UNPACK_SRCA
	emit 0x36c00000 /* release/flip after readback, matching UNPACR bank flip */
	jal ra, drain
.else
.ifdef TT_UNPACK_SRCB
	emit 0x36c00000
	jal ra, drain
.else
	/* Preserve identical code/park addresses across the three variants. */
	nop
	nop
	nop
.endif
.endif
finish_fixture:
.ifdef TT_RUNTIME_MATVEC
	addi s9, s9, 1
	addi s10, s10, 64
	li s4, 0
	bne s9, s11, fixture
.endif
.ifndef TT_UNPACK_GEMV
	addi s4, s4, 1
	sw s4, 4(s3)
	lw t0, 4(s3)
	andi t0, t0, 0
	li t0, 2
	bne s4, t0, fixture
.endif
	jal ra, drain
final_wait_return:
	li t0, 0x4d415431
	sw t0, 8(s3)
	lw t1, 8(s3)
	andi t1, t1, 0
	fence
parked:
	j parked
drain:
	sw ra, 0(s3)
	lw t1, 0(s3)
	andi t1, t1, 0
	fence
	sw zero, 0(s1)
wait_zero:
	lw t1, 0(s1)
	bne t1, zero, wait_zero
	/* Block the following ThCon marker until CFGEXU, both unpackers,
	 * FPU and SFPU are idle. No source-valid wait or PCBuf access.
	 */
	emit 0xa2101816
	emit 0x4500017e /* SETDMAREG low half of GPR63 = 1 */
wait_done:
	lw t1, 0(s1)
	beq t1, zero, wait_done
	fence
	jalr zero, 0(ra)
.ifdef TT_UNPACK_NONUNIFORM
.ifdef TT_RUNTIME_MATVEC
	.org 0x500, 0
	.global runtime_descriptor
runtime_descriptor:
	.word 0,0,0
	.org 0x540, 0
.else
	.org 0x440, 0
.endif
nonuniform_inputs:
.ifdef TT_RUNTIME_MATVEC
.ifdef TT_RUNTIME_Q8
	j q8_save_scales
q8_save_scales_return:
.endif
	li t3, 0x80000
	slli t0, s4, 5
	add t3, t3, t0
.else
	li t3, 0x70500
.endif
.ifndef TT_UNPACK_GEMV
	beq s4, zero, copy_nonuniform
	addi t3, t3, 256
.endif
copy_nonuniform:
	li t2, 0x70920
.ifdef TT_RUNTIME_MATVEC
	li t1, 8
.else
	li t1, 64
.endif
copy_nonuniform_word:
	lw t0, 0(t3)
	sw t0, 0(t2)
	addi t3, t3, 4
	addi t2, t2, 4
	addi t1, t1, -1
	bne t1, zero, copy_nonuniform_word
	j inputs_ready
.ifdef TT_UNPACK_FULL_BF16
.ifdef TT_RUNTIME_MATVEC
	.org 0x600, 0
.else
	.org 0x480, 0
.endif
full_bf16_math:
	li t2, 0
full_bf16_phase:
	li t0, 0xb20b0000 /* ThreadConfig FIDELITY_BASE.Phase, documented index 11 */
	or t0, t0, t2
	sw t0, 0(s0)
	jal ra, drain /* serialize CFGEXU before the corresponding FPU operation */
	li t0, 0x26000020 /* retain both operands until the final fidelity phase */
	li t3, 3
	bne t2, t3, full_bf16_emit
	li t0, 0x26c00020
full_bf16_emit:
	sw t0, 0(s0)
	jal ra, drain
	addi t2, t2, 1
	li t3, 4
	bne t2, t3, full_bf16_phase
	j full_bf16_return
.endif
.macro bf16_small value
.ifdef TT_UNPACK_RESIDENT
	.short 0 /* L1 poison: usable inputs must arrive from GDDR, not this image. */
.else
.if (\value) == -8
	.short 0xc100
.elseif (\value) == -7
	.short 0xc0e0
.elseif (\value) == -6
	.short 0xc0c0
.elseif (\value) == -5
	.short 0xc0a0
.elseif (\value) == -4
	.short 0xc080
.elseif (\value) == -3
	.short 0xc040
.elseif (\value) == -2
	.short 0xc000
.elseif (\value) == -1
	.short 0xbf80
.elseif (\value) == 0
	.short 0
.elseif (\value) == 1
	.short 0x3f80
.elseif (\value) == 2
	.short 0x4000
.elseif (\value) == 3
	.short 0x4040
.elseif (\value) == 4
	.short 0x4080
.elseif (\value) == 5
	.short 0x40a0
.elseif (\value) == 6
	.short 0x40c0
.elseif (\value) == 7
	.short 0x40e0
.else
	.error "fixed BF16 input outside small-integer whitelist"
.endif
.endif
.endm
.ifndef TT_RUNTIME_MATVEC
	.org 0x500, 0
.set nu_f, 0
.rept 2
.set nu_r, 0
.rept 8
.set nu_c, 0
.rept 16
.set nu_b, ((nu_r*3+nu_c)%16)-8
.if nu_f
.set nu_b, -nu_b-1
.endif
	bf16_small nu_b
.set nu_c, nu_c+1
.endr
.set nu_r, nu_r+1
.endr
.set nu_f, nu_f+1
.endr
	.org 0x710, 0
.set nu_r, 0
.rept 16
.set nu_c, 0
.rept 16
.ifdef TT_UNPACK_DENSE
.set nu_a, ((nu_r+2*nu_c)%5)-2
.else
.set nu_a, 0
.if nu_r == nu_c
.set nu_a, 1
.endif
.endif
	bf16_small nu_a
.set nu_c, nu_c+1
.endr
.set nu_r, nu_r+1
.endr
.endif
	/* Runtime weights and vector are supplied independently, not embedded. */
.endif
	/* This exact fixed region also contains the two local input buffers.
	 * No instructions may be placed in either buffer by the linker.
	 */
.ifdef TT_UNPACK_RESIDENT
	.org 0xa40, 0
	.global resident_fetch, resident_fault_park
resident_fetch:
	/* Initiator 0 is fixed/configured/verified by the host before start.
	 * Only this doorbell is device-written. Both fixtures fetch the same
	 * resident 1088-byte A/B data block; no host data write between them.
	 */
	li s5, 0xffb20000
	lw t0, 0x40(s5)
	bne t0, zero, resident_fault
.ifdef TT_UNPACK_GEMV
.ifdef TT_RUNTIME_MATVEC
	sw a0, 0(s5)
	lw t0, 0(s5)
	andi t0, t0, 0
.else
	/* Fixed GDDR base + chunk*1088, no code-supplied external address.
	 * Current s4 is bounded by the trusted shape before this code runs.
	 */
	slli t0, s4, 10
	slli t1, s4, 6
	add t0, t0, t1
	li t1, 0x04000000
	add t0, t0, t1
	sw t0, 0(s5)
	lw t0, 0(s5)
	andi t0, t0, 0
.endif
.endif
	lw s6, 0x208(s5)
	addi s6, s6, 1
	li t0, 1
	sw t0, 0x40(s5)
	lw t0, 0x40(s5) /* required doorbell readback before counter reads */
	andi t0, t0, 0
	li s7, 65536
resident_poll:
	lw t0, 0x40(s5)
	bne t0, zero, resident_next
	lw t0, 0x208(s5)
	bne t0, s6, resident_next
	lw t0, 0x240(s5)
	andi t0, t0, 255
	beq t0, zero, resident_ready
resident_next:
	addi s7, s7, -1
	bne s7, zero, resident_poll
resident_fault:
	li t0, 0x4e4f4346 /* NOCF; never a successful completion marker */
	sw t0, 8(s3)
	lw t0, 8(s3)
	andi t0, t0, 0
	fence
resident_fault_park:
	j resident_fault_park /* no reset, retry, counter clear or payload write */
resident_ready:
	fence /* invalidate RISC L0 before consuming externally written L1 */
.ifdef TT_RUNTIME_MATVEC
	addi a0, a0, 576 /* header16 + BF16 weight tile512 + padding48 */
.endif
	j nonuniform_inputs
.ifdef TT_RUNTIME_Q8
	.org 0xb00, 0
q8_clear:
	andi t0, s4, 1
	bne t0, zero, gemv_keep_accumulator
	j clear_first
q8_save_scales:
	/* The odd tile's 32-byte padding contains sixteen FP16 dW scales.
	 * Preserve it before the normal vector copy overwrites that padding.
	 * 70680..7069f is fixed private data, inside the saved code page. */
	andi t0, s4, 1
	beq t0, zero, q8_save_scales_return
	li t2, 0x70920
	li t3, 0x70680
	li t1, 8
q8_save_scale_word:
	lw t0, 0(t2)
	sw t0, 0(t3)
	addi t2, t2, 4
	addi t3, t3, 4
	addi t1, t1, -1
	bne t1, zero, q8_save_scale_word
	j q8_save_scales_return
q8_accumulate:
	andi t0, s4, 1
	beq t0, zero, q8_accumulate_return
	li t0, 1
	bne s4, t0, q8_have_accumulator
	emit 0x700b0000 /* LReg0/1 = zero, independent of stale Dst contents */
	emit 0x701b0000
	jal ra, drain
q8_have_accumulator:
	/* Convert each FP16 dW to FP32 with integer RISC instructions, including
	 * signed zero and subnormal half scales (do NOT flush these to zero). */
	li a2, 0x70680
	li a3, 0xffbd8000
	li a4, 0
q8_scale_half:
	lw a5, 0(a2)
	andi t0, a4, 1
	beq t0, zero, q8_scale_low
	srli a5, a5, 16
	addi a2, a2, 4
q8_scale_low:
	li t0, 0xffff
	and a5, a5, t0
	srli a6, a5, 10
	andi a6, a6, 31
	andi a7, a5, 1023
	li t0, 0x8000
	and a5, a5, t0
	slli a5, a5, 16
	bne a6, zero, q8_scale_normal
	beq a7, zero, q8_scale_store
	li a6, 113
q8_scale_denormal:
	andi t0, a7, 1024
	bne t0, zero, q8_scale_denormal_done
	slli a7, a7, 1
	addi a6, a6, -1
	j q8_scale_denormal
q8_scale_denormal_done:
	andi a7, a7, 1023
	j q8_scale_exponent
q8_scale_normal:
	addi a6, a6, 112
q8_scale_exponent:
	slli a6, a6, 23
	slli a7, a7, 13
	or a5, a5, a6
	or a5, a5, a7
q8_scale_store:
	sw a5, 0(a3)
	addi a3, a3, 4
	addi a4, a4, 1
	li t0, 16
	bne a4, t0, q8_scale_half
	/* One FP32 dX per 32-value input block, fixed saved gap 83000. */
	srli t0, s4, 1
	slli t0, t0, 2
	li t2, 0x83000
	add t2, t2, t0
	lw t0, 0(t2)
	li t2, 0xffbd8100 /* row4, sixteen identical scales */
	li t1, 16
q8_broadcast_scale:
	sw t0, 0(t2)
	addi t2, t2, 4
	addi t1, t1, -1
	bne t1, zero, q8_broadcast_scale
	lw t0, -4(t2)
	andi t0, t0, 0
	fence
	emit 0x70240020 /* signed-magnitude int32 dot: even/odd columns */
	emit 0x70340022
	emit 0x90000220 /* SFPCAST LReg2 -> LReg2, deterministic RNE */
	emit 0x90000330
	emit 0x70430000 /* FP32 dW even/odd, dX */
	emit 0x70530002
	emit 0x70630004
	emit 0x84046940 /* LReg4 = dW_even*dX + constant LReg9(+0) */
	emit 0x84056950
	emit 0x84024000 /* accumulator_even = dot_even*scale_even + accumulator_even */
	emit 0x84035110
	jal ra, drain
	addi t0, s4, 1
	bne t0, s8, q8_accumulate_return
	emit 0x72030020 /* final FP32 result to Dst row32, even/odd columns */
	emit 0x72130022
	jal ra, drain
	j q8_accumulate_return
.endif
	.org 0x1000, 0
.else
	.org 0xa20, 0
.endif
