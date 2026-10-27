/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j scalar_entry
.section .text.start,"ax"
.global scalar_entry,mixed_projection_wait,mixed_projection_wait_load,mixed_projection_wait_branch,scalar_fault_park,mixed_projection_fault_stopped
scalar_entry:
 li sp,0x70000
 call mixed_projection_main
 j scalar_fault_park
.org 0x40
mixed_projection_wait:
 li t0,0x75400
 mv t1,a0
mixed_projection_wait_load:
 lw a0,0(t0)
mixed_projection_wait_branch:
 beq a0,t1,mixed_projection_wait_load
 fence
 ret
.org 0x80
scalar_fault_park:
 li t0,0x75448
 li t1,0x46505251
 sw t1,0(t0)
 lw t1,0(t0)
 fence
mixed_projection_fault_stopped: j mixed_projection_fault_stopped
 j mixed_projection_fault_stopped
