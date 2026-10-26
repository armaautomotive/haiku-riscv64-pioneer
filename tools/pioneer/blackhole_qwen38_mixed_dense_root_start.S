/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j scalar_entry
.section .text.start,"ax"
.global scalar_entry,mixed_root_wait,mixed_root_wait_load,mixed_root_wait_branch,scalar_fault_park,mixed_root_fault_stopped
scalar_entry:
 li sp,0x70000
 call mixed_root_main
 j scalar_fault_park
.org 0x40
mixed_root_wait:
 li t0,0x75400
 mv t1,a0
mixed_root_wait_load:
 lw a0,0(t0)
mixed_root_wait_branch:
 beq a0,t1,mixed_root_wait_load
 fence
 ret
.org 0x80
scalar_fault_park:
 li t0,0x75448
 li t1,0x46524d51
 sw t1,0(t0)
 lw t1,0(t0)
 fence
mixed_root_fault_stopped: j mixed_root_fault_stopped
 j mixed_root_fault_stopped
