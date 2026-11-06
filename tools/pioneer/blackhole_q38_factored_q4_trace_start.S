/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j factored_trace_entry
.section .text.start,"ax"
.global factored_trace_entry,factored_trace_done,factored_trace_park,factored_trace_fault,factored_trace_fault_park
factored_trace_entry:
 li sp,0x70000
 call factored_trace_main
factored_trace_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,factored_trace_done
 sw t1,0(t0)
 li t1,0x31525151
 sw t1,8(t0)
 lw t1,8(t0)
 fence
factored_trace_park: j factored_trace_park
 j factored_trace_park
factored_trace_fault:
 li t0,0x17ffc8
 li t1,0x45525151
 sw t1,0(t0)
 fence
factored_trace_fault_park: j factored_trace_fault_park
 j factored_trace_fault_park
