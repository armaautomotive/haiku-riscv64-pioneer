/* SPDX-License-Identifier: MIT */
/* Offline-only entry/park witnesses; no reset or memory ownership is provided. */
#include "PioneerQwen35DataflowRoleAsmConstants.h"

.macro ROLE_START name, tick, ram_copy
.section .text.\name,"ax",@progbits
.globl \name
.type \name,@function
\name:
    /* Blackhole cfg0.DisLowCash: bypass the non-coherent L0 data cache.
       This is required before the first shared mailbox or queue read. */
    fence iorw,iorw
    csrsi 0x7c0,8
    csrr t0,0x7c0
    andi t0,t0,8
    beqz t0,2f                 /* do not access shared state if bypass failed */
    /* Copy the original local RAM before using it as a stack. Local loads
       stall while hardware is initializing RAM after reset; a remote NoC
       load would not. The host owns and saves each distinct L1 destination. */
    li t0,PIONEER_Q35_ROLE_PRIVATE_RAM_BASE
    li t1,\ram_copy
    li t2,PIONEER_Q35_ROLE_PRIVATE_STACK_TOP
3:
    lw t3,0(t0)
    sw t3,0(t1)
    addi t0,t0,4
    addi t1,t1,4
    bne t0,t2,3b
    fence rw,rw
    lw t3,-4(t1)               /* wait for the copied L1 writes to be visible */
    mv zero,t3
    li sp,PIONEER_Q35_ROLE_PRIVATE_STACK_TOP
    addi sp,sp,-16
    sw zero,0(sp)               /* private DataflowRoleTickState */
1:
    li a0,PIONEER_Q35_ROLE_MAILBOX_ADDRESS
    mv a1,sp
    call \tick
    beqz a0,2f                 /* malformed state: retain, then park */
    li t0,PIONEER_Q35_ROLE_STOP_REQUESTED
    bne a0,t0,1b
2:
    fence rw,rw
.globl \name\()_park
\name\()_park:
    j \name\()_park
.size \name, .-\name
.endm

ROLE_START qwen35_shared_reader_start, qwen35_shared_reader_tick, PIONEER_Q35_ROLE_READER_RAM_COPY
ROLE_START qwen35_shared_writer_start, qwen35_shared_writer_tick, PIONEER_Q35_ROLE_WRITER_RAM_COPY
