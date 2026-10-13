/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef PIONEER_KEXEC_PROBE_UART_H
#define PIONEER_KEXEC_PROBE_UART_H

#include "format.h"

#define PROBE_UART_BASE 0x7040000000ULL
#define PROBE_UART_POLL_LIMIT 262144U

ProbeU32 probe_mmio_read32(ProbeU64 address);
void probe_mmio_write32(ProbeU64 address, ProbeU32 value);
int probe_uart_try_putchar(int character);

#endif
