/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j q4_fpu_block_entry
.section .text.start,"ax"
.global q4_fpu_block_entry,q4_fpu_block_done,q4_fpu_block_park,q4_fpu_block_fault,q4_fpu_block_fault_park
q4_fpu_block_entry:
 li sp,0x70000
 call q4_fpu_block_main
q4_fpu_block_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,q4_fpu_block_done
 sw t1,0(t0)
 li t1,0x31504651 /* QFP1:new pipeline,not old BFP/GDDR marker */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
q4_fpu_block_park: j q4_fpu_block_park
 j q4_fpu_block_park
q4_fpu_block_fault:
 li t0,0x17ffc8
 li t1,0x45504651 /* QFPE:unknown,never authorizes cleanup */
 sw t1,0(t0)
 fence
q4_fpu_block_fault_park: j q4_fpu_block_fault_park
 j q4_fpu_block_fault_park
