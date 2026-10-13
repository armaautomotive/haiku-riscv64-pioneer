/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef PIONEER_KEXEC_PROBE_RUNTIME_H
#define PIONEER_KEXEC_PROBE_RUNTIME_H

#include "format.h"

struct ProbeSbiResult { long error; long value; };
void probe_putchar(int character);
struct ProbeSbiResult probe_sbi_read(unsigned long extension,
	unsigned long function, unsigned long argument);
void probe_halt(void) __attribute__((noreturn));
void probe_main(unsigned long hart, unsigned long fdt, unsigned long satp,
	unsigned long status, unsigned long interruptEnable,
	unsigned long interruptPending, unsigned long trapVector);
void probe_trap(unsigned long cause, unsigned long pc, unsigned long address);
extern unsigned char _probe_image_start[], _probe_image_end[];

#endif
