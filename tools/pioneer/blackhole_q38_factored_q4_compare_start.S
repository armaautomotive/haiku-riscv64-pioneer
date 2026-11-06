/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j factored_compare_entry
.section .text.start,"ax"
.global factored_compare_entry,factored_compare_done,factored_compare_park,factored_compare_fault,factored_compare_fault_park
factored_compare_entry:
 li sp,0x70000
 call factored_compare_main
factored_compare_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,factored_compare_done
 sw t1,0(t0)
 li t1,0x31435151
 sw t1,8(t0)
 lw t1,8(t0)
 fence
factored_compare_park: j factored_compare_park
 j factored_compare_park
factored_compare_fault:
 li t0,0x17ffc8
 li t1,0x45435151
 sw t1,0(t0)
 fence
factored_compare_fault_park: j factored_compare_fault_park
 j factored_compare_fault_park
