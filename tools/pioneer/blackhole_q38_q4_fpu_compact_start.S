/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j q4_fpu_compact_entry
.section .text.start,"ax"
.global q4_fpu_compact_entry,q4_fpu_compact_done,q4_fpu_compact_park,q4_fpu_compact_fault,q4_fpu_compact_fault_park
q4_fpu_compact_entry:
 li sp,0x70000
 call q4_fpu_compact_main
q4_fpu_compact_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,q4_fpu_compact_done
 sw t1,0(t0)
 li t1,0x31434651 /* QFC1:distinct compact original metadata pipeline */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
q4_fpu_compact_park: j q4_fpu_compact_park
 j q4_fpu_compact_park
q4_fpu_compact_fault:
 li t0,0x17ffc8
 li t1,0x45434651 /* QFCE:unknown,never cleanup authority */
 sw t1,0(t0)
 fence
q4_fpu_compact_fault_park: j q4_fpu_compact_fault_park
 j q4_fpu_compact_fault_park
