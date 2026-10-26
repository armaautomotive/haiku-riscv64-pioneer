/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j cohort_entry
.section .text.start,"ax"
.global cohort_entry,cohort_done,cohort_park,cohort_park_frontend,cohort_fault_park
cohort_entry:
 li sp,0x70000
 call q38_resident_advance_main
cohort_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,cohort_done
 sw t1,0(t0)
 li t1,0x31504151
 sw t1,8(t0)
 lw t1,8(t0)
 fence
cohort_park: j cohort_park
cohort_park_frontend: j cohort_park
.org 0x60
cohort_fault_park:
 li t0,0x17ffc8
 li t1,0x46504151
 sw t1,0(t0)
 lw t1,0(t0)
 fence
1: j 1b
