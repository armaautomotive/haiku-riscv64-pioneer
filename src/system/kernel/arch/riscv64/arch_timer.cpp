/*
 * Copyright 2019-2021, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
 */


#include <arch_cpu_defs.h>
#include <arch_int.h>
#include <arch/timer.h>
#include <boot/kernel_args.h>
#include <debug.h>
#include <kernel.h>
#include <platform/sbi/sbi_syscalls.h>
#include <timer.h>
#include <Clint.h>

#include <smp.h>


extern uint32 gPlatform;

static uint64 sTimerConversionFactor;
static bool sFirstTimerProgramming = true;


static void
TraceTimerMessage(const char* message)
{
	while (*message != '\0') {
		if (*message == '\n')
			sbi_console_putchar_legacy('\r');
		sbi_console_putchar_legacy(*message++);
	}
}


static void
TraceTimerValue(const char* label, uint64 value)
{
	char message[64];
	char* cursor = message;

	while (*label != '\0')
		*cursor++ = *label++;

	*cursor++ = '0';
	*cursor++ = 'x';
	for (int32 shift = 60; shift >= 0; shift -= 4) {
		uint8 digit = (value >> shift) & 0xf;
		*cursor++ = digit < 10 ? '0' + digit : 'a' + digit - 10;
	}
	*cursor++ = '\n';
	*cursor = '\0';

	TraceTimerMessage(message);
}


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
/*
	dprintf("arch_timer_set_hardware_timer(%" B_PRIu64 "), cpu: %" B_PRId32 "\n", timeout,
		smp_get_current_cpu());
*/
	uint64 scaledTimeout
		= (static_cast<__uint128_t>(timeout) * sTimerConversionFactor) >> 32;
	const uint64 now = CpuTime();
	const uint64 deadline = now + scaledTimeout;
	const bool firstProgramming = sFirstTimerProgramming;
	if (firstProgramming) {
		sFirstTimerProgramming = false;
		TraceTimerMessage("riscv: first timer programming entered\n");
		TraceTimerValue("riscv: timer timeout ", timeout);
		TraceTimerValue("riscv: timer factor ", sTimerConversionFactor);
		TraceTimerValue("riscv: timer now ", now);
		TraceTimerValue("riscv: timer deadline ", deadline);
		TraceTimerValue("riscv: timer sstatus ", Sstatus());
		TraceTimerValue("riscv: timer sie before ", Sie());
		TraceTimerValue("riscv: timer sip before ", Sip());
	}

	SetBitsSie(1 << sTimerInt);

	switch (gPlatform) {
		case kPlatformMNative:
			MSyscall(kMSyscallSetTimer, true, gClintRegs->mtime + scaledTimeout);
			break;
		case kPlatformSbi: {
			const sbiret result = sbi_set_timer(deadline);
			if (firstProgramming) {
				TraceTimerValue("riscv: timer result error ", result.error);
				TraceTimerValue("riscv: timer result value ", result.value);
			}
			if (result.error != SBI_SUCCESS) {
				TraceTimerMessage("riscv: modern SBI timer failed; using legacy call\n");
				sbi_set_timer_legacy(deadline);
			}
			break;
		}
		default:
			;
	}

	if (firstProgramming) {
		TraceTimerValue("riscv: timer sie after ", Sie());
		TraceTimerValue("riscv: timer sip after ", Sip());
		TraceTimerMessage("riscv: first timer programming returned\n");
	}
}


void
arch_timer_clear_hardware_timer()
{
	ClearBitsSie(1 << sTimerInt);

	switch (gPlatform) {
		case kPlatformMNative:
			MSyscall(kMSyscallSetTimer, false);
			break;
		case kPlatformSbi: {
			// Do nothing, it is not possible to clear SBI timer, so we only disable supervisor
			// timer interrupt. SBI timer still can be triggered, but its interrupt will be not
			// delivered to kernel.
			break;
		}
		default:
			;
	}
}


int
arch_init_timer(kernel_args *args)
{
	sTimerConversionFactor = (1LL << 32) * args->arch_args.timerFrequency / 1000000LL;

	return B_OK;
}
