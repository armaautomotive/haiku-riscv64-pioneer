/* SPDX-License-Identifier: MIT */
/* Dedicated-device role entry, compiled offline until takeover and recovery
   own the entire idle fleet. This entry intentionally discards private RAM;
   it must never be used by the old snapshot-and-restore role lifecycle. */
#include "PioneerQwen35DataflowRoleAsmConstants.h"

.macro ROLE_START name, tick
.section .text.\name,"ax",@progbits
.globl \name
.type \name,@function
\name:
    /* Shared L1 mailboxes are not coherent with the baby-RISC data cache. */
    fence iorw,iorw
    csrsi 0x7c0,8
    csrr t0,0x7c0
    andi t0,t0,8
    beqz t0,2f

    /* This RISC's local RAM is automatically stalled during hardware RAM
       initialization on reset release. Initialize only our own state, via
       MEM_LOCAL_BASE, before calling a routine that may use the stack. */
    li sp,PIONEER_Q35_ROLE_PRIVATE_STACK_TOP
    addi sp,sp,-16
    sw zero,0(sp)
1:
    li a0,PIONEER_Q35_ROLE_MAILBOX_ADDRESS
    mv a1,sp
    call \tick
    beqz a0,2f
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
