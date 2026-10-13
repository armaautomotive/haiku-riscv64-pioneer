/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "../kexec_probe/runtime.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

unsigned char _probe_image_start[1], _probe_image_end[1];
static _Alignas(8) unsigned char fdt[256];
static char output[8192];
static unsigned long used;
static unsigned int queries;
static int failBase;
static int failHsm;
static int activeSecondary;
static jmp_buf halted;

void
probe_putchar(int character)
{
	assert(used + 1 < sizeof(output));
	output[used++] = character;
	output[used] = 0;
}

struct ProbeSbiResult
probe_sbi_read(unsigned long extension, unsigned long function, unsigned long argument)
{
	queries++;
	if (extension == 0x48534d) {
		assert(function == 2 && argument < 64);
		struct ProbeSbiResult result = {0,
			argument == 3 || (activeSecondary && argument == 9) ? 0 : 1};
		return result;
	}
	assert(extension == 0x10);
	if (function == 0) {
		struct ProbeSbiResult result = {failBase ? -2 : 0, 0x1000000};
		return result;
	}
	assert(function == 4 || (function == 3
		&& (argument == 0x48534d || argument == 0x52464e43)));
	struct ProbeSbiResult result = {0,
		function == 4 ? 0x5b7 : (failHsm && argument == 0x48534d ? 0 : 1)};
	return result;
}

void __attribute__((noreturn))
probe_halt(void)
{
	longjmp(halted, 1);
}

int
probe_pioneer_ram_contains(ProbeU64 address, ProbeU64 size)
{
	/* Only replace the physical-address gate; production header checks run. */
	return address == (unsigned long)fdt && size > 0 && size <= sizeof(fdt);
}

static void
put32(unsigned int offset, ProbeU32 value)
{
	fdt[offset] = value >> 24;
	fdt[offset + 1] = value >> 16;
	fdt[offset + 2] = value >> 8;
	fdt[offset + 3] = value;
}

static void
reset(void)
{
	memset(fdt, 0, sizeof(fdt));
	put32(0, 0xd00dfeed);
	put32(4, sizeof(fdt));
	put32(8, 56);
	put32(12, 128);
	put32(16, 40);
	put32(20, 17);
	put32(24, 16);
	put32(28, 3);
	put32(32, 64);
	put32(36, 64);
	used = 0;
	output[0] = 0;
	queries = 0;
	failBase = 0;
	failHsm = 0;
	activeSecondary = 0;
}

static void
run(unsigned long hart, unsigned long pointer, unsigned long satp,
	unsigned long status, const char* expected)
{
	if (setjmp(halted) == 0) {
		probe_main(hart, pointer, satp, status, 0x222, 0, 0x1234);
		assert(!"probe returned without halting");
	}
	assert(strstr(output, "PIONEER_KEXEC_PROBE v1 BEGIN") != 0);
	assert(strstr(output, expected) != 0);
}

int
main(void)
{
	reset();
	run(3, (unsigned long)fdt, 0, 0, "END status=PASS (entry/header/SBI state only)");
	assert(queries == 68);
	assert(strstr(output, "fdt_header=OK (full tree not parsed)") != 0);
	reset();
	run(3, (unsigned long)fdt, 1, 0, "FAIL reason=MMU was not disabled");
	assert(queries == 0);
	reset();
	run(3, (unsigned long)fdt, 0, 2, "FAIL reason=supervisor interrupts");
	reset();
	run(64, (unsigned long)fdt, 0, 0, "FAIL reason=hart ID");
	reset();
	run(3, 0, 0, 0, "FAIL reason=FDT pointer");
	reset();
	run(3, (unsigned long)fdt + 1, 0, 0, "FAIL reason=FDT pointer");
	reset();
	put32(0, 0);
	run(3, (unsigned long)fdt, 0, 0, "FAIL reason=bad FDT magic");
	reset();
	put32(4, 512);
	run(3, (unsigned long)fdt, 0, 0, "FAIL reason=declared FDT extends");
	reset();
	failBase = 1;
	run(3, (unsigned long)fdt, 0, 0, "FAIL reason=SBI Base query failed");
	reset();
	failHsm = 1;
	run(3, (unsigned long)fdt, 0, 0, "FAIL reason=SBI HSM status queries unavailable");
	reset();
	activeSecondary = 1;
	run(3, (unsigned long)fdt, 0, 0, "FAIL reason=SBI hart states do not match");
	assert(strstr(output, "unexpected_hart=0x0000000000000009") != 0);
	reset();
	if (setjmp(halted) == 0) {
		probe_trap(15, 0x2200000, 0xcccccccc);
		assert(!"trap reporter returned without halting");
	}
	assert(strstr(output, "PIONEER_KEXEC_PROBE v1 TRAP") != 0);
	assert(strstr(output, "stval=0x00000000cccccccc") != 0);
	assert(strstr(output, "FAIL reason=unexpected supervisor trap") != 0);
	puts("PASS: production entry flow, fail-closed paths and trap reporting (mock SBI)");
	return 0;
}
