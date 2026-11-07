/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j native_bfp_band_entry
.section .text.start,"ax"
.global native_bfp_band_entry,native_bfp_band_done,native_bfp_band_park,native_bfp_band_fault,native_bfp_band_fault_park
native_bfp_band_entry:
 li sp,0x70000
 call native_bfp_band_main
native_bfp_band_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,native_bfp_band_done
 sw t1,0(t0)
 li t1,0x3442464e /* NFB4:distinct OFFBOARD image identity */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
native_bfp_band_park: j native_bfp_band_park
 j native_bfp_band_park
native_bfp_band_fault:
 li t0,0x17ffc8
 li t1,0x4542464e /* uncertain,not completion or cleanup grant */
 sw t1,0(t0)
 fence
native_bfp_band_fault_park: j native_bfp_band_fault_park
 j native_bfp_band_fault_park
