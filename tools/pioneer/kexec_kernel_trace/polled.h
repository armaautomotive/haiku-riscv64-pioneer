/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_KERNEL_TRACE_POLLED_H
#define PIONEER_KERNEL_TRACE_POLLED_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stddef.h>
#endif

struct PioneerKernelUartOps {
	unsigned int (*read)(void *context, unsigned int offset);
	void (*write)(void *context, unsigned int offset, unsigned int value);
	void (*delay)(void *context);
};

static inline int
pioneer_kernel_token_valid(const char *token, size_t length)
{
	size_t i;
	if (!token || length != 32)
		return 0;
	for (i = 0; i < length; i++)
		if (!((token[i] >= '0' && token[i] <= '9')
			|| (token[i] >= 'a' && token[i] <= 'f')))
			return 0;
	return 1;
}

/* No sleeping, allocation, clock dependency or register reconfiguration.
 * One bounded budget covers the entire message and final TX drain. A failed
 * UART never changes the traced kernel's execution path.
 */
static inline int
pioneer_kernel_uart_emit(const struct PioneerKernelUartOps *ops, void *context,
	const char *message, size_t length)
{
	unsigned int budget = 20000;
	size_t i;
	if (!ops || !ops->read || !ops->write || !ops->delay || !message
		|| !length || length > 191)
		return 0;
	for (i = 0; i < length; i++) {
		if (ops->read(context, 12) & 0x80)
			return 0;
		while (!(ops->read(context, 20) & 0x20)) {
			if (!budget--)
				return 0;
			ops->delay(context);
		}
		ops->write(context, 0, (unsigned char)message[i]);
	}
	while (!(ops->read(context, 20) & 0x40)) {
		if (!budget--)
			return 0;
		ops->delay(context);
	}
	return 1;
}
#endif
