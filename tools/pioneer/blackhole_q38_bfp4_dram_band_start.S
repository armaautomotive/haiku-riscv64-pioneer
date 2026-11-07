/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j native_dram_band_entry
.section .text.start,"ax"
.global native_dram_band_entry,native_dram_band_done,native_dram_band_park,native_dram_band_fault,native_dram_band_fault_park
native_dram_band_entry:
 li sp,0x70000
 call native_dram_band_main
native_dram_band_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,native_dram_band_done
 sw t1,0(t0)
 li t1,0x34534442 /* BDS4:timed GDDR BFP4,not old native band */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
native_dram_band_park: j native_dram_band_park
 j native_dram_band_park
native_dram_band_fault:
 li t0,0x17ffc8
 li t1,0x4542444e /* fault:uncertain,NO reset/restore permission */
 sw t1,0(t0)
 fence
native_dram_band_fault_park: j native_dram_band_fault_park
 j native_dram_band_fault_park
