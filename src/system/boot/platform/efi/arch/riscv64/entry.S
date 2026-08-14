/*
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

    .text

/*  status_t arch_enter_kernel(addr_t satp, struct kernel_args *kernelArgs,
        addr_t kernelEntry, addr_t kernelStackTop);

    a0  - SATP register value
    a1  - kernelArgs
    a2  - kernelEntry
    a3  - kernelStackTop
*/
FUNCTION(arch_enter_kernel):
	// EFI may leave supervisor timer/external interrupts enabled. The Haiku
	// kernel must begin with them masked until its PLIC and timer setup runs.
	csrci sstatus, 0x2
	csrw sie, zero
	csrw satp, a0
	sfence.vma
	// The Pioneer UART is identity inaccessible after SATP is enabled. Use its
	// kernel virtual mapping to leave handoff progress markers on the serial
	// console without disturbing the argument registers.
	li t0, 0xffffffc0068ac000
	li t2, 'A'
	sw t2, 0(t0)

	// set the kernel stack
	mv	sp, a3
	li	fp, 0
	li  ra, 0

	li t2, 'B'
	sw t2, 0(t0)

	// Setup kernel args
	mv	a0, a1	// kernelArgs
	mv	t0, a2
	li	a1, 0	// currentCPU=0

	li t3, 0xffffffc0068ac000
	li t2, 'C'
	sw t2, 0(t3)

	// call the kernel
	jr	t0

	// return
	li	a0,-1  // B_ERROR
	ret
FUNCTION_END(arch_enter_kernel)
