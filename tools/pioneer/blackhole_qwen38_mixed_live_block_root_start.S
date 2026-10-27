/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j scalar_entry
.section .text.start,"ax"
.global scalar_entry,mixed_live_block_wait,mixed_live_block_wait_load,mixed_live_block_wait_branch,mixed_live_block_stopped,scalar_fault_park,mixed_live_block_fault_stopped
scalar_entry:
 li sp,0x70000
 call mixed_live_block_main
 j scalar_fault_park
.org 0x40
mixed_live_block_wait:
 li t0,0x75400
 mv t1,a0
mixed_live_block_wait_load:
 lw a0,0(t0)
mixed_live_block_wait_branch:
 beq a0,t1,mixed_live_block_wait_load
 fence
 ret
.org 0x60
mixed_live_block_stopped: j mixed_live_block_stopped
 j mixed_live_block_stopped
.org 0x80
scalar_fault_park:
 li t0,0x75448
 li t1,0x464c4251
 sw t1,0(t0)
 lw t1,0(t0)
 fence
mixed_live_block_fault_stopped: j mixed_live_block_fault_stopped
 j mixed_live_block_fault_stopped
