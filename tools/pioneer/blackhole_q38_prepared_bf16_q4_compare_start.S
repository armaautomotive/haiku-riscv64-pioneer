/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j prepared_bf16_compare_entry
.section .text.start,"ax"
.global prepared_bf16_compare_entry,prepared_bf16_compare_done,prepared_bf16_compare_park,prepared_bf16_compare_fault,prepared_bf16_compare_fault_park
prepared_bf16_compare_entry:
 li sp,0x70000
 call prepared_bf16_compare_main
prepared_bf16_compare_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,prepared_bf16_compare_done
 sw t1,0(t0)
 li t1,0x31425151
 sw t1,8(t0)
 lw t1,8(t0)
 fence
prepared_bf16_compare_park: j prepared_bf16_compare_park
 j prepared_bf16_compare_park
prepared_bf16_compare_fault:
 li t0,0x17ffc8
 li t1,0x45425151
 sw t1,0(t0)
 fence
prepared_bf16_compare_fault_park: j prepared_bf16_compare_fault_park
 j prepared_bf16_compare_fault_park
