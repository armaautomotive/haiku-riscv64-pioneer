/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j exact_q4_affine_entry
.section .text.start,"ax"
.global exact_q4_affine_entry,exact_q4_affine_done,exact_q4_affine_park,exact_q4_affine_fault,exact_q4_affine_fault_park
exact_q4_affine_entry:
 li sp,0x70000
 call exact_q4_affine_main
exact_q4_affine_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,exact_q4_affine_done
 sw t1,0(t0)
 li t1,0x31413451 /* Q4A1:full-K affine arithmetic,NOT model completion */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
exact_q4_affine_park: j exact_q4_affine_park
 j exact_q4_affine_park
exact_q4_affine_fault:
 li t0,0x17ffc8
 li t1,0x46413451 /* Q4AF:unknown,NO next job/cleanup */
 sw t1,0(t0)
 fence
exact_q4_affine_fault_park: j exact_q4_affine_fault_park
 j exact_q4_affine_fault_park
