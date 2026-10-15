/* SPDX-License-Identifier: MIT */
/* Offline test image only. No driver operation installs or starts this image.
 * RV32I: no stack, local RAM, CSR, NoC command, DMA or coprocessor access.
 * The sole stores are to two words in the previously tested L1 tail.
 */
.option norvc
.option norelax
.section .boot,"ax",@progbits
.global pioneer_brisc_boot
pioneer_brisc_boot:
	j pioneer_brisc_start

.section .text,"ax",@progbits
.global pioneer_brisc_start
pioneer_brisc_start:
	lui t0, 0x180
	addi t0, t0, -64            /* L1 0x17ffc0, not a hardware mailbox FIFO */
	addi t1, zero, 0
	addi t2, zero, 1
	addi t3, zero, 33
1:
	add t1, t1, t2
	addi t2, t2, 1
	blt t2, t3, 1b
	sw t1, 0(t0)                /* 1 + ... + 32 = 528 */
	lw t4, 0(t0)
	addi zero, t4, 0            /* consume readback before publishing done */
	fence
	lui t1, 0x50495
	addi t1, t1, -178           /* 0x50494f4e, "PION" */
	sw t1, 4(t0)
	lw t4, 4(t0)
	addi zero, t4, 0
	fence
.global pioneer_brisc_park
pioneer_brisc_park:
	j pioneer_brisc_park
