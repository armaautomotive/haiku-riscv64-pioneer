/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.ifndef TT_ATTENTION_DATA_ONLY
.section .boot,"ax"
.global _start
_start:
	j attention_entry
.section .text.start,"ax"
.global attention_entry,attention_done,attention_park,attention_fault_park
attention_entry:
	li sp,0x78000
	call attention_main
attention_done:
	li t0,0x17ffc0
	sw a0,4(t0)
	la t1,attention_done
	sw t1,0(t0)
	li t1,0x314e5441 /* ATN1: bounded result, 0 means rejected inputs */
	sw t1,8(t0)
	lw t1,8(t0)
	fence
.ifdef TT_ATTENTION_COORDINATED
	/* Only a separately qualified dispatch image may enter this variant.
	 * Full source/NoC proof and mailbox publication remain in the prelude. */
	li t0,0x71200
	jr t0
.endif
attention_park:
	j attention_park
attention_fault_park:
	li t0,0x17ffc8
	li t1,0x464e5441 /* ATNF: no valid completion/stop authorization */
	sw t1,0(t0)
	lw t1,0(t0)
	fence
1: j 1b
.endif

.section .rodata.attention,"a"
.balign 4
.global attention_exact_begin,attention_exact_end
attention_exact_begin:
	/* Copied verbatim from our qualified q8_exact_sequence, not host code. */
	.word 0x84001930
	.word 0x7170ffff,0x717af000
	.word 0x7e000741,0x7e001751
	.word 0x850a0462,0x850a1572
	.word 0x84045302
	.word 0x84065000,0x84047000,0x84067000
	.word 0x850a3210,0x850a1342
	.word 0x850a1452,0x850a3552
	.word 0x850a2462,0x850a5620
	.word 0x850a0230,0x850a3042
	.word 0x850a3452,0x850a0552
	.word 0x850a2462,0x850a5600
	.word 0x71220001,0x7e003261
	.word 0x7d000051
	.word 0x7c000340,0x8d000040
	.word 0x7afe1443,0x7a001441,0x79001445
	.word 0x8a001002,0x7b000502,0x7b000606
	.word 0x79000434
	.word 0x8a000002
	.word 0x850a1300
attention_exact_end:
