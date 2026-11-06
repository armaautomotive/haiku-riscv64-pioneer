/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j factored_q4_entry
.section .text.start,"ax"
.global factored_q4_entry,factored_q4_done,factored_q4_park,factored_q4_fault,factored_q4_fault_park
factored_q4_entry:
 li sp,0x70000
 call factored_q4_main
factored_q4_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,factored_q4_done
 sw t1,0(t0)
 li t1,0x31465151
 sw t1,8(t0)
 lw t1,8(t0)
 fence
factored_q4_park: j factored_q4_park
 j factored_q4_park
factored_q4_fault:
 li t0,0x17ffc8
 li t1,0x45465151
 sw t1,0(t0)
 fence
factored_q4_fault_park: j factored_q4_fault_park
 j factored_q4_fault_park
