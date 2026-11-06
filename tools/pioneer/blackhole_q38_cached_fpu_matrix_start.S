/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j cached_fpu_entry
.section .text.start,"ax"
.global cached_fpu_entry,cached_fpu_done,cached_fpu_park,cached_fpu_fault,cached_fpu_fault_park
cached_fpu_entry:
 li sp,0x70000
 call cached_fpu_main
cached_fpu_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,cached_fpu_done
 sw t1,0(t0)
 li t1,0x314d4351 /* distinct returning service harness */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
cached_fpu_park: j cached_fpu_park
 j cached_fpu_park
cached_fpu_fault:
 li t0,0x17ffc8
 li t1,0x454d4351 /* QCSE:unknown,never cleanup authority */
 sw t1,0(t0)
 fence
cached_fpu_fault_park: j cached_fpu_fault_park
 j cached_fpu_fault_park
