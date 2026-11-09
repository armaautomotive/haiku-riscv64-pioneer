/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j srca_probe_entry
.section .text.start,"ax"
.global srca_probe_entry,srca_probe_done,srca_probe_park,srca_probe_fault,srca_probe_fault_park
srca_probe_entry:
 li sp,0x70000
 call srca_probe_main
srca_probe_done:
 li t0,0x17ffc0
 sw a0,4(t0)
 la t1,srca_probe_done
 sw t1,0(t0)
 li t1,0x31504353 /* SCP1: probe only */
 sw t1,8(t0)
 lw t1,8(t0)
 fence
srca_probe_park: j srca_probe_park
 j srca_probe_park
srca_probe_fault:
 li t0,0x17ffc8
 li t1,0x46504353 /* SCPF: uncertain; no retry or cleanup */
 sw t1,0(t0)
 fence
srca_probe_fault_park: j srca_probe_fault_park
 j srca_probe_fault_park
