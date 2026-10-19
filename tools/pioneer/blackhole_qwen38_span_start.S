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
	li t1,0x31553851 /* Q8U1:OFFLINE seven-bank span ONLY */
	sw t1,8(t0)
	lw t1,8(t0)
	fence
block_park: j block_park
	j block_park /* speculative next-fetch stays safe,not fault entry */
.org 0x60
block_fault_park:
	li t0,0x17ffc8
	li t1,0x46553851 /* Q8UF:UNKNOWN,no subsequent packet/reset/restore */
	sw t1,0(t0)
	lw t1,0(t0)
	fence
1: j 1b
