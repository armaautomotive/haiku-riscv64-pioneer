/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "runtime.h"
#include "uart.h"

int
probe_uart_try_putchar(int character)
{
	/* Live Pioneer FDT: dw-apb-uart, reg-shift 2, reg-io-width 4.
	 * Preserve baud, clocks, FIFO and interrupt settings. Refuse a divisor
	 * bank rather than accidentally overwriting DLL instead of the TX byte.
	 */
	if ((probe_mmio_read32(PROBE_UART_BASE + 12) & 0x80) != 0)
		return 0;
	for (unsigned int count = 0; count < PROBE_UART_POLL_LIMIT; count++) {
		if ((probe_mmio_read32(PROBE_UART_BASE + 20) & 0x20) != 0) {
			probe_mmio_write32(PROBE_UART_BASE, (unsigned char)character);
			return 1;
		}
	}
	return 0;
}

void
probe_putchar(int character)
{
	/* No PASS can be generated after a console timeout. No SBI-console call
	 * is made, so unavailable legacy output cannot block later diagnostics.
	 */
	if (!probe_uart_try_putchar(character))
		probe_halt();
}
