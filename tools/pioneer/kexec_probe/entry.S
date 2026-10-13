/* Copyright 2026. Distributed under the terms of the MIT License. */

.section .text.head,"ax",@progbits
.option norelax
.option norvc
.globl _probe_image_start
.type _probe_image_start,@function
_probe_image_start:
	j _probe_entry
	nop
	.quad 0x200000
	.quad _probe_image_end - _probe_image_start
	.quad 0
	.word 2
	.word 0
	.quad 0
	.ascii "RISCV\0\0\0"
	.word 0x05435352
	.word 0

.globl _probe_entry
_probe_entry:
	/* a0/a1 are the Linux boot ABI's hart ID and physical FDT pointer. */
	csrr a2, satp
	csrr a3, sstatus
	csrr a4, sie
	csrr a5, sip
	csrr a6, stvec
	lla sp, _probe_stack_top
	lla t0, _probe_trap_entry
	csrw stvec, t0
	li t0, 2
	csrc sstatus, t0
	csrw sie, zero
	fence.i
	call probe_main
	j probe_halt

.balign 4
_probe_trap_entry:
	li t0, 2
	csrc sstatus, t0
	csrw sie, zero
	csrr a0, scause
	csrr a1, sepc
	csrr a2, stval
	lla sp, _probe_trap_stack_top
	call probe_trap
	j probe_halt

.globl probe_putchar
.type probe_putchar,@function
probe_putchar:
	li a7, 1
	ecall
	ret
.size probe_putchar, .-probe_putchar

.globl probe_sbi_read
.type probe_sbi_read,@function
probe_sbi_read:
	/* Callers use only Base queries and HSM get-status, never mutations. */
	mv a7, a0
	mv a6, a1
	mv a0, a2
	li a1, 0
	li a2, 0
	li a3, 0
	li a4, 0
	li a5, 0
	ecall
	ret
.size probe_sbi_read, .-probe_sbi_read

.globl probe_halt
.type probe_halt,@function
probe_halt:
	/* Do not reset automatically or depend on WFI/interrupt behavior. */
1:	j 1b
.size probe_halt, .-probe_halt

.section .data.stack,"aw",@progbits
.balign 16
	.space 4096
.globl _probe_trap_stack_top
_probe_trap_stack_top:
	.space 16384
.globl _probe_stack_top
_probe_stack_top:
