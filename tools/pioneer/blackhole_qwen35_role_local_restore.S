/* SPDX-License-Identifier: MIT */
/* Offline-only, stackless B/NC local-RAM restore witnesses. The host must
   own and validate the immutable L1 source, boot image, reset and routes.
   Receipt values are not a substitute for those ownership proofs. */
#include "PioneerQwen35DataflowRoleAsmConstants.h"

.macro ROLE_RESTORE name, ram_copy, ready_offset, success, failure
.section .text.\name,"ax",@progbits
.globl \name
.type \name,@function
\name:
    fence iorw,iorw
    csrsi 0x7c0,8
    csrr t0,0x7c0
    andi t0,t0,8
    beqz t0,3f                 /* no access if cache bypass is unavailable */
    li t0,\ram_copy
    li t1,PIONEER_Q35_ROLE_PRIVATE_RAM_BASE
    li t2,PIONEER_Q35_ROLE_PRIVATE_STACK_TOP
1:
    lw t3,0(t0)
    sw t3,0(t1)
    addi t0,t0,4
    addi t1,t1,4
    bne t1,t2,1b
    fence rw,rw
    li t0,\ram_copy
    li t1,PIONEER_Q35_ROLE_PRIVATE_RAM_BASE
2:
    lw t3,0(t0)
    lw t4,0(t1)
    bne t3,t4,4f
    addi t0,t0,4
    addi t1,t1,4
    bne t1,t2,2b
    li t1,\success
    j 5f
4:
    li t1,\failure
5:
    li t0,PIONEER_Q35_ROLE_SNAPSHOT_HANDOFF
    sw t1,\ready_offset(t0)
    fence rw,rw
    lw t3,\ready_offset(t0)
    mv zero,t3                 /* receipt store is observed before park */
3:
    fence rw,rw
.globl \name\()_park
\name\()_park:
    j \name\()_park
.size \name, .-\name
.endm

/* These receipts are private to this experimental restore witness. */
ROLE_RESTORE qwen35_restore_reader_start, PIONEER_Q35_ROLE_READER_RAM_COPY, 0, 0x81, 0xe1
ROLE_RESTORE qwen35_restore_writer_start, PIONEER_Q35_ROLE_WRITER_RAM_COPY, 4, 0x82, 0xe2
