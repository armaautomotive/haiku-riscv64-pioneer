/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j scalar_entry
.section .text.start,"ax"
.global scalar_entry,mixed_recurrent_block_wait,mixed_recurrent_block_wait_load,mixed_recurrent_block_wait_branch,mixed_recurrent_block_stopped,scalar_fault_park,mixed_recurrent_block_fault_stopped
scalar_entry:
 li sp,0x70000
 call mixed_recurrent_block_main
 j scalar_fault_park
.org 0x40
mixed_recurrent_block_wait:
 li t0,0x75400
 mv t1,a0
mixed_recurrent_block_wait_load:
 lw a0,0(t0)
mixed_recurrent_block_wait_branch:
 beq a0,t1,mixed_recurrent_block_wait_load
 fence
 ret
.org 0x60
mixed_recurrent_block_stopped: j mixed_recurrent_block_stopped
 j mixed_recurrent_block_stopped
.org 0x80
scalar_fault_park:
 li t0,0x75448
 li t1,0x46425251
 sw t1,0(t0)
 lw t1,0(t0)
 fence
mixed_recurrent_block_fault_stopped: j mixed_recurrent_block_fault_stopped
 j mixed_recurrent_block_fault_stopped
