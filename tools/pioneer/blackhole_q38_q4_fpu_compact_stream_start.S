/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j q4_fpu_compact_stream_entry
.section .text.start,"ax"
.global q4_fpu_compact_stream_entry,q4_fpu_compact_stream_done,q4_fpu_compact_stream_park,q4_fpu_compact_stream_fault,q4_fpu_compact_stream_fault_park
q4_fpu_compact_stream_entry:
 li sp,0x70000
 call q4_fpu_compact_stream_main
q4_fpu_compact_stream_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,q4_fpu_compact_stream_done
 sw t1,0(t0)
.ifdef PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE
 li t1,0x31544351 /* QCT1:distinct timed diagnostic,NOT104 */
.else
 li t1,0x31534351 /* QCS1:NOT old85 or old84 */
.endif
 sw t1,8(t0)
 lw t1,8(t0)
 fence
q4_fpu_compact_stream_park: j q4_fpu_compact_stream_park
 j q4_fpu_compact_stream_park
q4_fpu_compact_stream_fault:
 li t0,0x17ffc8
.ifdef PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE
 li t1,0x45544351 /* QCTE:unknown,never cleanup authority */
.else
 li t1,0x45534351 /* QCSE:unknown,never cleanup authority */
.endif
 sw t1,0(t0)
 fence
q4_fpu_compact_stream_fault_park: j q4_fpu_compact_stream_fault_park
 j q4_fpu_compact_stream_fault_park

