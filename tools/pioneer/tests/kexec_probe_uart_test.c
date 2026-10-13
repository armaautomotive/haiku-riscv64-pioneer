/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "../kexec_probe/uart.h"
#include "../kexec_probe/runtime.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

static unsigned int polls, writes, readyAfter;
static ProbeU32 bank, lastByte;
static jmp_buf halted;

ProbeU32
probe_mmio_read32(ProbeU64 address)
{
	if (address == PROBE_UART_BASE + 12)
		return bank;
	assert(address == PROBE_UART_BASE + 20);
	return polls++ >= readyAfter ? 0x20 : 0;
}

void
probe_mmio_write32(ProbeU64 address, ProbeU32 value)
{
	assert(address == PROBE_UART_BASE);
	writes++;
	lastByte = value;
}

void __attribute__((noreturn))
probe_halt(void)
{
	longjmp(halted, 1);
}

int
main(void)
{
	assert(probe_uart_try_putchar('P'));
	assert(polls == 1 && writes == 1 && lastByte == 'P');
	polls = writes = 0;
	readyAfter = 3;
	assert(probe_uart_try_putchar(0x1ff));
	assert(polls == 4 && writes == 1 && lastByte == 0xff);
	polls = writes = 0;
	bank = 0x80;
	assert(!probe_uart_try_putchar('P'));
	assert(polls == 0 && writes == 0);
	bank = 0;
	readyAfter = PROBE_UART_POLL_LIMIT;
	assert(!probe_uart_try_putchar('P'));
	assert(polls == PROBE_UART_POLL_LIMIT && writes == 0);
	polls = 0;
	if (setjmp(halted) == 0) {
		probe_putchar('P');
		assert(!"failed console output did not halt");
	}
	assert(polls == PROBE_UART_POLL_LIMIT && writes == 0);
	puts("PASS: UART register bounds, ready polling, divisor-bank refusal and timeout halt");
	return 0;
}
