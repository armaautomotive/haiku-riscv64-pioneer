/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j native_bfp_entry
.section .text.start,"ax"
.global native_bfp_entry,native_bfp_done,native_bfp_park,native_bfp_fault,native_bfp_fault_park
native_bfp_entry:
 li sp,0x70000
 call native_bfp_main
native_bfp_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,native_bfp_done
 sw t1,0(t0)
.ifdef PIONEER_Q38_EXACT_Q4_CORRECTED_IMAGE
 li t1,0x31433451 /* Q4C1:corrected integer dot,NOT full-model completion */
.else
.ifdef PIONEER_Q38_EXACT_Q4_PRIMARY_IMAGE
 li t1,0x31503451 /* Q4P1:primary dot ONLY,not old BFP/fullQ4 completion */
.else
 li t1,0x3142464e /* NFB1:new BFP prototype,not an old completion */
.endif
.endif
 sw t1,8(t0)
 lw t1,8(t0)
 fence
native_bfp_park: j native_bfp_park
 j native_bfp_park
native_bfp_fault:
 li t0,0x17ffc8
.ifdef PIONEER_Q38_EXACT_Q4_CORRECTED_IMAGE
 li t1,0x46433451 /* Q4CF:uncertain,no next job/IO/cleanup */
.else
.ifdef PIONEER_Q38_EXACT_Q4_PRIMARY_IMAGE
 li t1,0x46503451 /* Q4PF:uncertain,no next job/IO/cleanup */
.else
 li t1,0x4642464e /* NFBF:uncertain,no next job/IO/cleanup */
.endif
.endif
 sw t1,0(t0)
 fence
native_bfp_fault_park: j native_bfp_fault_park
 j native_bfp_fault_park
