/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j native_dram_load_entry
.section .text.start,"ax"
.global native_dram_load_entry,native_dram_load_done,native_dram_load_park,native_dram_load_fault,native_dram_load_fault_park
native_dram_load_entry:
 li sp,0x70000
 call native_dram_load_main
native_dram_load_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,native_dram_load_done
 sw t1,0(t0)
 li t1,0x314c444e /* NDL1:LOAD ONLY,not a numerical/FPU result */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
native_dram_load_park: j native_dram_load_park
 j native_dram_load_park
native_dram_load_fault:
 li t0,0x17ffc8
 li t1,0x454c444e /* NDLE:unknown,no retry/cleanup authority */
 sw t1,0(t0)
 fence
native_dram_load_fault_park: j native_dram_load_fault_park
 j native_dram_load_fault_park
