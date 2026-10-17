/* SPDX-License-Identifier: MIT */
/* Fixed TRISC0 prelude for the immutable bulk Q8 program at70000.
 * No host DMA: peers pull the shared activation from owned worker0 L1.
 * Input7050c=0(master)/1(peer),70510=fixed worker's translated NoC XY.
 * The host has proved all workers held, input DMA complete and BME off.
 * On any ambiguous transfer park forever with NOCF; no retry/reset.
 */
.option norvc
.option norelax
.section .text,"ax"
.global fleet_start,fleet_fault_park,fleet_end
fleet_start:
.ifdef TT_FLEET_PERSISTENT
	li a0,0x71300
	li t0,0x50525331
	sw t0,12(a0)
	fence
	j fleet_persistent_wait
.endif
fleet_job:
	li a0,0x70500
	lw t0,12(a0)
.ifdef TT_FLEET_PERSISTENT
	beq t0,zero,fleet_weights
.else
	beq t0,zero,fleet_poison
.endif
	li t1,1
	bne t0,t1,fleet_fault
	li a1,0xffb20000
	lw t0,0x40(a1)
	bne t0,zero,fleet_fault
	li t0,0x80000
	sw t0,0(a1)
	sw zero,4(a1)
	li t1,129 /* source worker0=(1,2), translated */
	sw t1,8(a1)
	sw t0,12(a1)
	sw zero,16(a1)
	lw t0,16(a0)
	sw t0,20(a1)
	sw zero,24(a1)
	sw zero,28(a1)
	li t0,16384
	sw t0,32(a1)
	sw zero,36(a1)
	lw t0,36(a1)
	andi t0,t0,0
	fence
	lw a2,0x208(a1)
	addi a2,a2,1
	lw a3,0x20c(a1)
	addi a3,a3,256
	lw a4,0x210(a1)
	addi a4,a4,1
	lw a5,0x214(a1)
	addi a5,a5,1
	li t0,1
	sw t0,0x40(a1)
	lw t0,0x40(a1)
	andi t0,t0,0
	li a6,65536
fleet_poll:
	lw t0,0x40(a1)
	bne t0,zero,fleet_next
	lw t0,0x208(a1)
	bne t0,a2,fleet_next
	lw t0,0x20c(a1)
	bne t0,a3,fleet_next
	lw t0,0x210(a1)
	bne t0,a4,fleet_next
	lw t0,0x214(a1)
	bne t0,a5,fleet_next
	lw t0,0x240(a1)
	andi t0,t0,255
	beq t0,zero,fleet_weights
fleet_next:
	addi a6,a6,-1
	bne a6,zero,fleet_poll
	j fleet_fault
fleet_weights:
	fence /* discard RISC L0 before any activation consumption */
	/* Reconfigure the same sole initiator for the unchanged bulk reader. */
.ifdef TT_FLEET_PERSISTENT
	li a1,0xffb20000
	lw t0,0x40(a1)
	bne t0,zero,fleet_fault
	sw zero,4(a1)
	sw zero,16(a1)
	lw t0,16(a0)
	sw t0,20(a1)
	sw zero,24(a1)
	sw zero,28(a1)
	sw zero,36(a1)
.endif
	lw t0,0(a0)
	sw t0,0(a1)
	li t0,786 /* qualified bank0=(18,12) */
	sw t0,8(a1)
	li t0,0x88000
	sw t0,12(a1)
	li t0,9216
	sw t0,32(a1)
	lw t0,32(a1)
	andi t0,t0,0
	fence
fleet_poison:
	/* Device-local poison replaces repeated host->card output-page DMAs.
	 * Every result must still be finite and every row/tile proved complete. */
	lw t0,8(a0)
	slli t0,t0,4
	li t1,0x84000
	li t2,0x7fc00000
fleet_poison_word:
	sw t2,0(t1)
	addi t1,t1,4
	addi t0,t0,-1
	bne t0,zero,fleet_poison_word
	fence
	li t0,0x70000
	jr t0
fleet_fault:
	li t0,0x17ffc0
	li t1,0x4e4f4346
	sw t1,8(t0)
	lw t1,8(t0)
	andi t1,t1,0
	fence
fleet_fault_park:
	j fleet_fault_park
.ifdef TT_FLEET_PERSISTENT
	/* No reset or host control transfer between jobs. The kernel owns
	 * both descriptors and monotonic doorbells; users cannot supply code.
	 * A math completion branches here only AFTER the existing full drain.
	 * FENCE invalidates the RISC data L0 before mailbox/activation loads. */
	.org 0x200,0
	.global fleet_persistent_done,fleet_persistent_wait,fleet_persistent_wait_end,fleet_persistent_stop_park
fleet_persistent_done:
	li a0,0x71300
	lw t0,0(a0)
	sw t0,4(a0)
	fence
fleet_persistent_wait:
	li a0,0x71300
	fence
	lw t0,0(a0)
	lw t1,4(a0)
	beq t0,t1,fleet_persistent_wait
	addi t1,t1,1
	bne t0,t1,fleet_fault
	lw t1,8(a0)
	beq t1,zero,fleet_job
	li t2,1
	bne t1,t2,fleet_fault
	sw t0,4(a0)
	li t0,0x53544f50
	sw t0,12(a0)
	fence
fleet_persistent_stop_park:
	j fleet_persistent_stop_park
fleet_persistent_wait_end:
	/* Data-only mailbox: issued, finished, stop request, ready marker. */
	.org 0x300,0
	.word 0,0,0,0
.endif
fleet_end:
