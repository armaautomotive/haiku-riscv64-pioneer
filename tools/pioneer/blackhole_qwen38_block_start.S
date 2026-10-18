/* SPDX-License-Identifier: MIT */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start: j block_entry
.section .text.start,"ax"
.global block_entry,block_done,block_park,block_fault_park
block_entry:
	li sp,0x70000
	call block_main
block_done:
	li t0,0x17ffc0
	sw a0,4(t0)
	la t1,block_done
	sw t1,0(t0)
.ifdef TT_Q38_SHARED_ROWS
	li t1,0x31483851 /* Q8H1:shared root inputs,NOT driver50 */
.else
.ifdef TT_Q38_ROWS
	li t1,0x31523851 /* Q8R1:OFFLINE fullK,multiple output row groups */
.else
.ifdef TT_Q38_STREAM
	li t1,0x31533851 /* Q8S1:OFFLINE fullK stream,not local block48 */
.else
	li t1,0x31423851 /* Q8B1:distinct from one-panel completion */
.endif
.endif
.endif
	sw t1,8(t0)
	lw t1,8(t0)
	fence
block_park: j block_park
block_fault_park:
	li t0,0x17ffc8
.ifdef TT_Q38_SHARED_ROWS
	li t1,0x46483851 /* Q8HF:NO retry/reset/restore after UNKNOWN */
.else
.ifdef TT_Q38_ROWS
	li t1,0x46523851 /* Q8RF:NO retry/reset/restore after uncertainty */
.else
.ifdef TT_Q38_STREAM
	li t1,0x46533851 /* Q8SF:NO retry/reset/restore after uncertain fetch */
.else
	li t1,0x46423851 /* Q8BF:NO success/stop permission */
.endif
.endif
.endif
	sw t1,0(t0)
	lw t1,0(t0)
	fence
1: j 1b
