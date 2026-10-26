/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j scalar_entry
.section .text.start,"ax"
.global scalar_entry,mixed_joined_wait,mixed_joined_wait_load,mixed_joined_wait_branch,scalar_fault_park,mixed_joined_fault_stopped
scalar_entry:
 li sp,0x70000
 call mixed_joined_main
 j scalar_fault_park
.org 0x40
mixed_joined_wait:
 li t0,0x75400
 mv t1,a0
mixed_joined_wait_load:
 lw a0,0(t0)
mixed_joined_wait_branch:
 beq a0,t1,mixed_joined_wait_load
 fence
 ret
.org 0x80
scalar_fault_park:
 li t0,0x75448
 li t1,0x46524a51
 sw t1,0(t0)
 lw t1,0(t0)
 fence
mixed_joined_fault_stopped: j mixed_joined_fault_stopped
 j mixed_joined_fault_stopped
