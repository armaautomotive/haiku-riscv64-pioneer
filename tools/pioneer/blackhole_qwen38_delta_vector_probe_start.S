/* SPDX-License-Identifier: MIT */
/* NEW image only:both sampled frontend terminal words branch to PARK.
 * Fault code is disjoint;NEVER widen acceptance for the old aliased image. */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j scalar_entry
.section .text.start,"ax"
.global scalar_entry,scalar_done,scalar_park,scalar_fault_park
scalar_entry:
 li sp,0x70000
 call scalar_main
scalar_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,scalar_done
 sw t1,0(t0)
 li t1,0x31564451
 sw t1,8(t0)
 lw t1,8(t0)
 fence
scalar_park: j scalar_park
 j scalar_park
.rept 8
 j scalar_park
.endr
scalar_fault_park:
 li t0,0x17ffc8
 li t1,0x46564451
 sw t1,0(t0)
 lw t1,0(t0)
 fence
1: j 1b
