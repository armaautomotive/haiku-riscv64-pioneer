/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j block_entry
.section .q38_global_start,"ax"
.global block_entry,block_peer_wait,block_peer_wait_load,block_peer_wait_branch
.global block_peer_stopped,block_fault_park,block_fault_stopped
block_entry:
 li sp,0x70000
 call q38_global_worker_main
 j block_fault_park
.org 0x40
block_peer_wait:
 li t0,0x75200
 mv t1,a0
block_peer_wait_load:
 lw a0,0(t0)
block_peer_wait_branch:
 beq a0,t1,block_peer_wait_load
 fence
 ret
.org 0x60
block_peer_stopped: j block_peer_stopped
 j block_peer_stopped
.org 0x80
block_fault_park:
 li t0,0x75248
 li t1,0x46574751
 sw t1,0(t0)
 lw t1,0(t0)
 fence
block_fault_stopped: j block_fault_stopped
 j block_fault_stopped
