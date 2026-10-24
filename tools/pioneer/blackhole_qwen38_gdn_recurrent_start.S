/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j scalar_entry
.section .text.start,"ax"
.global scalar_entry,scalar_done,scalar_park,scalar_fault_park
scalar_entry:
 li sp,0x70000
 call scalar_main
scalar_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,scalar_done
 sw t1,0(t0)
 li t1,0x314a5251 /* QRJ1:combined local preparation+state step,NOT native/token commit */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
scalar_park: j scalar_park
scalar_fault_park:
 li t0,0x17ffc8
 li t1,0x464a5251
 sw t1,0(t0)
 lw t1,0(t0)
 fence
1: j 1b
