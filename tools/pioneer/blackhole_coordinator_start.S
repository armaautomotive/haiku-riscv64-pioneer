/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j coordinator_entry
.section .text.start,"ax"
.global coordinator_entry,coordinator_done,coordinator_park,coordinator_fault_park
coordinator_entry:
	li sp,0x70000
	call coordinator_main
coordinator_done:
	li t0,0x17ffc0
	sw a0,4(t0)
	la t1,coordinator_done
	sw t1,0(t0)
	li t1,0x31445243 /* CRD1 */
	sw t1,8(t0)
	lw t1,8(t0)
	fence
coordinator_park: j coordinator_park
coordinator_fault_park:
	li t0,0x17ffc8
	li t1,0x46445243 /* CRDF: NO completion or reset authorization */
	sw t1,0(t0)
	lw t1,0(t0)
	fence
1: j 1b
