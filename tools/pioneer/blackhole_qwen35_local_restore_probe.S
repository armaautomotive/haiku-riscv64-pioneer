/* SPDX-License-Identifier: MIT */
/* Restricted qualification only. Each B/NC saves and verifies its private
   RAM, waits for the host to validate the L1 copy, flips and reads back one
   private word, then jumps to the separately built restore image. No stack
   or inference code is used. An invalid command or failed check parks. */
#include "PioneerQwen35DataflowRoleAsmConstants.h"

.macro PROBE_START name, ram_copy, ready_offset, ready_value, restore_code
.section .text.\name,"ax",@progbits
.globl \name
.type \name,@function
\name:
    fence iorw,iorw
    csrsi 0x7c0,8
    csrr t0,0x7c0
    andi t0,t0,8
    beqz t0,9f
    li t0,PIONEER_Q35_ROLE_PRIVATE_RAM_BASE
    li t1,\ram_copy
    li t2,PIONEER_Q35_ROLE_PRIVATE_STACK_TOP
1:
    lw t3,0(t0)
    sw t3,0(t1)
    addi t0,t0,4
    addi t1,t1,4
    bne t0,t2,1b
    fence rw,rw
    lw t3,-4(t1)
    mv zero,t3
    li t0,PIONEER_Q35_ROLE_PRIVATE_RAM_BASE
    li t1,\ram_copy
2:
    lw t3,0(t0)
    lw t4,0(t1)
    bne t3,t4,9f
    addi t0,t0,4
    addi t1,t1,4
    bne t0,t2,2b
    li t0,PIONEER_Q35_ROLE_SNAPSHOT_HANDOFF
    li t1,\ready_value
    sw t1,\ready_offset(t0)
3:
    lw t2,8(t0)
    beqz t2,3b
    li t1,PIONEER_Q35_ROLE_SNAPSHOT_CANCEL
    bne t2,t1,9f
    /* This write is deliberately observable and reversible. If the
       readback fails, do not publish a completion or use a stack. */
    li t1,PIONEER_Q35_ROLE_PRIVATE_RAM_BASE
    lw t3,0(t1)
    xori t4,t3,1
    sw t4,0(t1)
    fence rw,rw
    lw t5,0(t1)
    bne t5,t4,9f
    li t0,\restore_code
    jr t0
9:
    fence rw,rw
.globl \name\()_park
\name\()_park:
    j \name\()_park
.size \name, .-\name
.endm

PROBE_START qwen35_restore_probe_reader_start, PIONEER_Q35_ROLE_READER_RAM_COPY, 0, PIONEER_Q35_ROLE_SNAPSHOT_READER_READY, 0xb0000
PROBE_START qwen35_restore_probe_writer_start, PIONEER_Q35_ROLE_WRITER_RAM_COPY, 4, PIONEER_Q35_ROLE_SNAPSHOT_WRITER_READY, PIONEER_Q35_ROLE_WRITER_CODE
