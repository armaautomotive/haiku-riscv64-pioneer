/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start:
	j q35_work_entry
.section .text.start,"ax"
.global q35_work_entry, q35_work_park, q35_work_fault_park, block_fault_park
q35_work_entry:
	li sp,0x70000
	call q35_work_main
	beqz a0,block_fault_park
	li t0,0x17ffc0
	sw a0,4(t0)
	li t1,0x31443551
	fence
	sw t1,0(t0)
	lw t1,0(t0)
	fence
q35_work_park:
	j q35_work_park
	j q35_work_park
block_fault_park:
	li t0,0x17ffc0
	sw zero,4(t0)
	li t1,0x31463551
	fence
	sw t1,0(t0)
	lw t1,0(t0)
	fence
q35_work_fault_park:
	j q35_work_fault_park
	j q35_work_fault_park
