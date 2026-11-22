/* SPDX-License-Identifier: MIT */
/* Offline-only entry/park witnesses; no reset or memory ownership is provided. */
#include "PioneerQwen35DataflowRoleAsmConstants.h"

.macro ROLE_START name, tick
.section .text.\name,"ax",@progbits
.globl \name
.type \name,@function
\name:
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

ROLE_START qwen35_shared_reader_start, qwen35_shared_reader_tick
ROLE_START qwen35_shared_writer_start, qwen35_shared_writer_tick
