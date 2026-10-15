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
main:
	li s0, 0xffe40000
	li s1, 0xffe000fc /* ThCon GPR63, shared with this TRISC0 */
	li s2, 0xffef0000
	li s3, 0x17ffc0
	li s4, 0
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
	config 0, 0x42b5
	config 1, 0x20aa0000
	config 2, 0
	config 3, 0 /* TRISC0 RISC Dst access = converted IEEE FP32 */
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
	config 64, 0x00100015
	config 65, 16
	config 72, 5
	config 73, 3
	config 76, 0x7070 /* payload/16 - 1 */
	config 84, 0x00400040
	config 86, 0x00100010
	config 112, 0x00100015
	config 113, 8
	config 120, 5
	config 121, 3
	config 124, 0x7091
	fence
	emit 0x5e23fc00 /* unpack0 X bounds 0..255 BF16 elements */
	emit 0x5e41fc00 /* unpack1 X bounds 0..127 BF16 elements */
	jal ra, drain
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
	li t0, 0x10180000 /* clear all Dst valid bits; no Dst input is in use */
	nop /* same width as the former two-word one-row clear immediate */
	li t2, 1
clear_output:
	sw t0, 0(s0)
	addi t0, t0, 1
	addi t2, t2, -1
	bne t2, zero, clear_output
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
	addi s4, s4, 1
	sw s4, 4(s3)
	lw t0, 4(s3)
	andi t0, t0, 0
	li t0, 2
	bne s4, t0, fixture
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
	.org 0x440, 0
nonuniform_inputs:
	li t3, 0x70500
	beq s4, zero, copy_nonuniform
	addi t3, t3, 256
copy_nonuniform:
	li t2, 0x70920
	li t1, 64
copy_nonuniform_word:
	lw t0, 0(t3)
	sw t0, 0(t2)
	addi t3, t3, 4
	addi t2, t2, 4
	addi t1, t1, -1
	bne t1, zero, copy_nonuniform_word
	j inputs_ready
.ifdef TT_UNPACK_FULL_BF16
	.org 0x480, 0
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
	j nonuniform_inputs
	.org 0x1000, 0
.else
	.org 0xa20, 0
.endif
