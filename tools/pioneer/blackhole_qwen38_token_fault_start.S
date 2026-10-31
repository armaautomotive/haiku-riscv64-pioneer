/* SPDX-License-Identifier: MIT */
/* Separate UNSELECTED diagnostic entry. No native/Work install capability. */
.section .boot,"ax",@progbits
.global _start
_start: j scalar_entry
.section .text.start,"ax",@progbits
.global scalar_entry
scalar_entry:
 li sp,0x70000
 call q38_global_chain_main
 j scalar_fault_park
.section .terminal,"ax",@progbits
.global scalar_fault_park
scalar_fault_park: j q38_token_fault_report
.global q38_global_chain_stopped
q38_global_chain_stopped: j q38_global_chain_stopped

.section .text.q38_token_fault,"ax",@progbits
.global q38_token_fault_report
.type q38_token_fault_report,@function
q38_token_fault_report:
 /* These offsets are checked against the actual C++ layout by the test.
  * ra is a saved LINK,not necessarily the failed guard/retired PC. Write
  * it before publishing the nonzero R::Status[2] error. Never overwrite
  * Status[3]'s startup marker,terminal DATA,NoC registers or model state. */
 lui t0,0x75
 sw ra,0x460(t0)
 lui t1,0x51465
 addi t1,t1,0x431
 fence
 sw t1,0x448(t0)
 fence
1: j 1b
.size q38_token_fault_report,.-q38_token_fault_report
