/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j bf16_producer_entry
.section .text.start,"ax"
.global bf16_producer_entry,bf16_producer_done,bf16_producer_park,bf16_producer_fault,bf16_producer_fault_park
bf16_producer_entry:
 li sp,0x70000
 call bf16_producer_main
bf16_producer_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,bf16_producer_done
 sw t1,0(t0)
 li t1,0x31494251 /* distinct returning service harness */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
bf16_producer_park: j bf16_producer_park
 j bf16_producer_park
bf16_producer_fault:
 li t0,0x17ffc8
 li t1,0x45494251 /* QCSE:unknown,never cleanup authority */
 sw t1,0(t0)
 fence
bf16_producer_fault_park: j bf16_producer_fault_park
 j bf16_producer_fault_park
