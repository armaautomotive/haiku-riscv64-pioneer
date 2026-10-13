/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "runtime.h"

_Static_assert(sizeof(unsigned long) == 8, "RV64 entry ABI required");

static void
text(const char* string)
{
	while (*string != 0) {
		if (*string == '\n')
			probe_putchar('\r');
		probe_putchar(*string++);
	}
}

static void
hex(const char* name, ProbeU64 value)
{
	const char* digits = "0123456789abcdef";
	text(name);
	text("=0x");
	for (int shift = 60; shift >= 0; shift -= 4)
		probe_putchar(digits[(value >> shift) & 15]);
	text("\n");
}

static void __attribute__((noreturn))
fail(const char* reason)
{
	text("PIONEER_KEXEC_PROBE v1 END status=FAIL reason=");
	text(reason);
	text("\n");
	probe_halt();
}

void
probe_main(unsigned long hart, unsigned long fdt, unsigned long satp,
	unsigned long status, unsigned long interruptEnable,
	unsigned long interruptPending, unsigned long trapVector)
{
	text("PIONEER_KEXEC_PROBE v1 BEGIN\n");
	hex("hart", hart);
	hex("fdt", fdt);
	hex("satp", satp);
	hex("sstatus", status);
	hex("sie", interruptEnable);
	hex("sip", interruptPending);
	hex("previous_stvec", trapVector);
	hex("image_start", (unsigned long)_probe_image_start);
	hex("image_end", (unsigned long)_probe_image_end);
	if (satp != 0)
		fail("MMU was not disabled at entry");
	if ((status & 2) != 0)
		fail("supervisor interrupts were enabled at entry");
	if (hart >= 64)
		fail("hart ID outside Pioneer range");
	if ((fdt & 7) != 0
		|| !probe_pioneer_ram_contains(fdt, PROBE_FDT_HEADER_SIZE))
		fail("FDT pointer outside permitted RAM/alignment");
	struct ProbeFdtHeader header;
	const char* error = probe_check_fdt_header((const unsigned char*)fdt,
		PROBE_FDT_HEADER_SIZE, &header);
	if (error != 0)
		fail(error);
	if (!probe_pioneer_ram_contains(fdt, header.totalSize))
		fail("declared FDT extends outside permitted RAM");
	hex("fdt_size", header.totalSize);
	hex("fdt_version", header.version);
	hex("fdt_boot_cpuid", header.bootCpu);
	text("fdt_header=OK (full tree not parsed)\n");
	struct ProbeSbiResult result = probe_sbi_read(0x10, 0, 0);
	hex("sbi_spec_error", result.error);
	hex("sbi_spec_version", result.value);
	if (result.error != 0)
		fail("SBI Base query failed");
	result = probe_sbi_read(0x10, 4, 0);
	hex("sbi_vendor_error", result.error);
	hex("sbi_vendor", result.value);
	result = probe_sbi_read(0x10, 3, 0x48534d);
	hex("sbi_hsm_error", result.error);
	hex("sbi_hsm_available", result.value);
	if (result.error != 0 || result.value == 0)
		fail("SBI HSM status queries unavailable");
	unsigned long started = 0, stopped = 0, unexpected = 0;
	for (unsigned long id = 0; id < 64; id++) {
		result = probe_sbi_read(0x48534d, 2, id);
		if (result.error == 0 && result.value == 0)
			started++;
		else if (result.error == 0 && result.value == 1)
			stopped++;
		if (result.error != 0 || result.value != (id == hart ? 0 : 1)) {
			hex("unexpected_hart", id);
			hex("hart_status_error", result.error);
			hex("hart_status", result.value);
			unexpected++;
		}
	}
	hex("sbi_harts_started", started);
	hex("sbi_harts_stopped", stopped);
	if (unexpected != 0)
		fail("SBI hart states do not match one active / 63 stopped");
	result = probe_sbi_read(0x10, 3, 0x52464e43);
	hex("sbi_rfence_error", result.error);
	hex("sbi_rfence_available", result.value);
	text("PIONEER_KEXEC_PROBE v1 END status=PASS (entry/header/SBI state only)\n");
	text("HALTED: recover with the approved relay off/wait/on sequence\n");
	probe_halt();
}

void
probe_trap(unsigned long cause, unsigned long pc, unsigned long address)
{
	text("PIONEER_KEXEC_PROBE v1 TRAP\n");
	hex("scause", cause);
	hex("sepc", pc);
	hex("stval", address);
	fail("unexpected supervisor trap");
}
