/*
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

    .text

/*  status_t arch_enter_kernel(uint64 satp, struct kernel_args *kernelArgs,
        addr_t kernelEntry, addr_t kernelStackTop);

    a0  - SATP register value
    a1  - kernelArgs
    a2  - kernelEntry
    a3  - kernelStackTop
*/
FUNCTION(arch_enter_kernel):
#ifdef PIONEER_KEXEC_PROBE
	// Kernel text was copied from the RAM filesystem after Linux's flush.
	fence.i
#endif
	csrw satp, a0
	sfence.vma

	// set the kernel stack
	mv	sp, a3
#	li	fp, 0
#	li  ra, 0

	// Setup kernel args
	mv	a0, a1	// kernelArgs
	mv	t0, a2
	li	a1, 0	// currentCPU=0

	// call the kernel
	jr	t0

	// return
	li	a0,-1  // B_ERROR
	ret
FUNCTION_END(arch_enter_kernel)

#ifdef PIONEER_KEXEC_PROBE
// SBI HSM: a0 physical hart, a1 physical PioneerApEntry*, satp=0, S-mode.
// No gp, C++ call, shared stack, or descriptor load after changing satp.
.balign 4
FUNCTION(pioneer_ap_entry):
	.option push
	.option norelax
	csrw sie, zero
	li t6, 2
	csrc sstatus, t6
	csrw sscratch, a1
	lla t6, .Lpioneer_ap_trap
	csrw stvec, t6
	fence.i
	csrr t6, satp
	bnez t6, .Lpioneer_ap_bad
	ld t6, 48(a1)
	bne a0, t6, .Lpioneer_ap_bad
	csrr t6, 0x5c0
	srli t6, t6, 21
	andi t6, t6, 1
	ld t0, 64(a1)
	bne t0, t6, .Lpioneer_ap_bad
	li t6, 0x1e000
	csrc sstatus, t6
	li t6, 0x2000
	csrs sstatus, t6
	li t6, 1
	sd t6, 56(a1)
	fence rw, rw
	ld t0, 0(a1)
	ld t1, 8(a1)
	ld t2, 16(a1)
	ld t3, 24(a1)
	ld t4, 32(a1)
	ld t5, 40(a1)
	mv t6, a1
	csrw satp, t0
	sfence.vma
	fence.i
	add sp, t1, t2
	li fp, 0
	// Touch our private final stack before acknowledging the handoff.
	sd zero, -8(sp)
	li a0, 2
	sd a0, 56(t6)
	fence rw, rw
	mv a0, t4
	mv a1, t5
	jr t3
.Lpioneer_ap_bad:
	li t6, 3
	sd t6, 56(a1)
	fence rw, rw
	j .Lpioneer_ap_park
.balign 4
.Lpioneer_ap_trap:
	csrr t6, sscratch
	csrr t0, scause
	sd t0, 72(t6)
	csrr t0, sepc
	sd t0, 80(t6)
	csrr t0, stval
	sd t0, 88(t6)
	li t0, 4
	sd t0, 56(t6)
	fence rw, rw
.Lpioneer_ap_park:
	wfi
	j .Lpioneer_ap_park
	.option pop
FUNCTION_END(pioneer_ap_entry)
#endif
