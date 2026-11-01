/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j dram_sweep_entry
.section .text.start,"ax"
.global dram_sweep_entry,dram_sweep_park,dram_sweep_fault_park,dram_sweep_fault_idle
dram_sweep_entry:
 li sp,0x70000
 call dram_sweep_main
dram_sweep_park: j dram_sweep_park
 j dram_sweep_park /* speculative next fetch is also inert */
dram_sweep_fault_park:
 li t0,0x74100
 li t1,0x46524244 /* DRBF DATA only, NOT safe-to-restore authority */
 sw t1,0(t0)
 lw t1,0(t0)
 fence
dram_sweep_fault_idle: j dram_sweep_fault_idle
 j dram_sweep_fault_idle
