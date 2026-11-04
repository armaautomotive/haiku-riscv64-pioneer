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
 .ifdef PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64
 li t1,0x3436444e /* ND64:separate LOAD-ONLY83 placement control */
 .else
 li t1,0x314c444e /* NDL1:LOAD ONLY,not a numerical/FPU result */
 .endif
 sw t1,8(t0)
 lw t1,8(t0)
 fence
native_dram_load_park: j native_dram_load_park
 j native_dram_load_park
native_dram_load_fault:
 li t0,0x17ffc8
 .ifdef PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64
 li t1,0x4536444e /* ND6E:unknown,no retry/cleanup authority */
 .else
 li t1,0x454c444e /* NDLE:unknown,no retry/cleanup authority */
 .endif
 sw t1,0(t0)
 fence
native_dram_load_fault_park: j native_dram_load_fault_park
 j native_dram_load_fault_park
