/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j q6_fpu_stream_entry
.section .text.start,"ax"
.global q6_fpu_stream_entry,q6_fpu_stream_done,q6_fpu_stream_park,q6_fpu_stream_fault,q6_fpu_stream_fault_park
q6_fpu_stream_entry:
 li sp,0x70000
 call q6_fpu_stream_main
q6_fpu_stream_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,q6_fpu_stream_done
 sw t1,0(t0)
 li t1,0x31533651 /* Q6S1:NOT old85 or old84 */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
q6_fpu_stream_park: j q6_fpu_stream_park
 j q6_fpu_stream_park
q6_fpu_stream_fault:
 li t0,0x17ffc8
 li t1,0x45533651 /* QFSE:unknown,never cleanup authority */
 sw t1,0(t0)
 fence
q6_fpu_stream_fault_park: j q6_fpu_stream_fault_park
 j q6_fpu_stream_fault_park
