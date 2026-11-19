/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .text.q35.dispatch.start,"ax"
.global q35_dispatch_matrix
.type q35_dispatch_matrix,@function
q35_dispatch_matrix:
	li t0,0x17ffb0
	li t1,0x54000000
	sw t1,0(t0)
	sw zero,4(t0)
	li t0,0xe7068
	li t1,0xd0000000
	sw t1,0(t0)
	fence
	tail q35_dispatch_body
.size q35_dispatch_matrix,.-q35_dispatch_matrix
