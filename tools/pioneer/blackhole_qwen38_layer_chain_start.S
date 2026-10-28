/* SPDX-License-Identifier: MIT */
.section .boot,"ax",@progbits
.global _start
_start: j scalar_entry
.section .text.start,"ax",@progbits
.global scalar_entry
scalar_entry:
 li sp,0x70000
 call q38_layer_chain_main
 j scalar_fault_park
.section .terminal,"ax",@progbits
.global scalar_fault_park
scalar_fault_park: j scalar_fault_park
.global q38_layer_chain_stopped
q38_layer_chain_stopped: j q38_layer_chain_stopped
