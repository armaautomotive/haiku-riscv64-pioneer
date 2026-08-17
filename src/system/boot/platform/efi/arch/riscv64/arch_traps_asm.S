/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <arch_traps.h>


.globl SVec
.type  SVec, @function
.align 4
SVec:
	// Make page faults visible even if the C trap handler cannot yet use the
	// post-EFI console.
	li t0, 0xffffffc07fffe000
	li t1, '!'
	sb t1, 0(t0)
	PushTrapFrame
	li t0, 0xffffffc07fffe000
	li t1, 'E'
	sb t1, 0(t0)

	// Do not call the C trap reporter yet: it faults recursively while the
	// kernel is still bringing up its first console. Emit the low nibbles of
	// scause and sepc directly, then stop so the original exception remains
	// observable on the Pioneer serial console.
	csrr t2, scause
	csrr t3, sepc
	andi t2, t2, 0xf
	andi t3, t3, 0xf
	li t4, 10
	bltu t2, t4, 1f
	addi t2, t2, 'a' - 10
	j 2f
1:
	addi t2, t2, '0'
2:
	bltu t3, t4, 3f
	addi t3, t3, 'a' - 10
	j 4f
3:
	addi t3, t3, '0'
4:
	li t0, 0xffffffc07fffe000
	sb t2, 0(t0)
	sb t3, 0(t0)
1:
	wfi
	j 1b
.size	SVec, .-SVec
