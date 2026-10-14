/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


	.section	.stack,"aw",@nobits
.globl gStack
.type  gStack, @object
gStack:
	.skip 1*1024*1024
.size	gStack, .-gStack
.globl gStackEnd
gStackEnd:

	.section	.text.head
	.option norelax
.globl _riscv_start
_riscv_start:
#ifdef PIONEER_KEXEC_PROBE
	.option push
	.option norvc
	j .Lpioneer_entry
	nop
	.quad 0x200000
	.quad _riscv_image_end - _riscv_start
	.quad 0
	.word 2
	.word 0
	.quad 0
	.ascii "RISCV\0\0\0"
	.word 0x05435352
	.word 0
.Lpioneer_entry:
	.option pop
	csrr a2, satp
	csrr a3, sstatus
	csrr a4, sie
	li t0, 2
	csrc sstatus, t0
	csrw sie, zero
	li t0, 0x6000
	csrc sstatus, t0
	li t0, 0x2000
	csrs sstatus, t0
	lla t0, .Lpioneer_trap
	csrw stvec, t0
#endif
	la sp, gStackEnd
	li fp, 0
#ifdef PIONEER_KEXEC_PROBE
	call pioneer_kexec_start
#else
	call _start
#endif
	j .

#ifdef PIONEER_KEXEC_PROBE
.balign 4
.Lpioneer_trap:
	csrr a0, scause
	csrr a1, sepc
	csrr a2, stval
	la sp, gStackEnd
	call pioneer_kexec_trap
	j .
#endif
