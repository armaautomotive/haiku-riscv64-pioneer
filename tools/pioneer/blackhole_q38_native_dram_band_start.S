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
.ifdef PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64
 li t1,0x3436424e /* NB64:distinct84 math,NOT load-only83 ND64 */
.else
 li t1,0x3142444e /* NDB1:original81 */
.endif
 sw t1,8(t0)
 lw t1,8(t0)
 fence
native_dram_band_park: j native_dram_band_park
 j native_dram_band_park
native_dram_band_fault:
 li t0,0x17ffc8
.ifdef PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64
 li t1,0x4546424e /* NBFE:uncertain;NO retry/restore authority */
.else
 li t1,0x4542444e /* NDBE */
.endif
 sw t1,0(t0)
 fence
native_dram_band_fault_park: j native_dram_band_fault_park
 j native_dram_band_fault_park
