/* SPDX-License-Identifier: MIT */
#include "../kexec_kernel_trace/polled.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct fixture {
	unsigned int reads, delays, writes;
	int mode;
	char output[256];
};

static unsigned int read_uart(void *context, unsigned int offset)
{
	struct fixture *f = context;
	f->reads++;
	assert(offset == 12 || offset == 20);
	if (offset == 12)
		return f->mode == 1 ? 0x80 : 0;
	if (f->mode == 2)
		return 0;
	if (f->mode == 3)
		return 0x20; /* Ready for bytes, but never completely drained. */
	if (f->mode == 4 && f->delays < 3)
		return 0;
	return 0x60;
}

static void write_uart(void *context, unsigned int offset, unsigned int value)
{
	struct fixture *f = context;
	assert(offset == 0 && value <= 255 && f->writes < 255);
	f->output[f->writes++] = (char)value;
}

static void delay_uart(void *context)
{
	((struct fixture *)context)->delays++;
}

int main(void)
{
	const struct PioneerKernelUartOps ops = { read_uart, write_uart, delay_uart };
	const char *message = "\r\nPIONEER_KERNEL_TRACE self-test\r\n";
	struct fixture f = {0};
	assert(pioneer_kernel_token_valid("0123456789abcdef0123456789abcdef", 32));
	assert(!pioneer_kernel_token_valid(NULL, 32));
	assert(!pioneer_kernel_token_valid("0123456789abcdef0123456789abcdef", 31));
	assert(!pioneer_kernel_token_valid("G123456789abcdef0123456789abcdef", 32));
	assert(pioneer_kernel_uart_emit(&ops, &f, message, strlen(message)));
	assert(f.writes == strlen(message) && !strcmp(f.output, message));
	f = (struct fixture){ .mode = 1 };
	assert(!pioneer_kernel_uart_emit(&ops, &f, message, strlen(message)) && !f.writes);
	f = (struct fixture){ .mode = 2 };
	assert(!pioneer_kernel_uart_emit(&ops, &f, message, strlen(message)));
	assert(!f.writes && f.delays == 20000 && f.reads < 20005);
	f = (struct fixture){ .mode = 3 };
	assert(!pioneer_kernel_uart_emit(&ops, &f, message, strlen(message)));
	assert(f.writes == strlen(message) && f.delays == 20000);
	f = (struct fixture){ .mode = 4 };
	assert(pioneer_kernel_uart_emit(&ops, &f, message, strlen(message)) && f.delays == 3);
	f = (struct fixture){0};
	assert(!pioneer_kernel_uart_emit(NULL, &f, message, strlen(message)));
	assert(!pioneer_kernel_uart_emit(&ops, &f, NULL, 10));
	assert(!pioneer_kernel_uart_emit(&ops, &f, message, 0));
	assert(!pioneer_kernel_uart_emit(&ops, &f, message, 192));
	assert(!f.reads && !f.writes);
	puts("kernel checkpoint token/UART bounds tests: PASS");
	return 0;
}
