/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "../kexec_probe/format.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void
put32(unsigned char* data, unsigned int offset, ProbeU32 value)
{
	data[offset] = value >> 24;
	data[offset + 1] = value >> 16;
	data[offset + 2] = value >> 8;
	data[offset + 3] = value;
}

static void
valid(unsigned char* data)
{
	memset(data, 0, PROBE_FDT_HEADER_SIZE);
	put32(data, 0, 0xd00dfeed);
	put32(data, 4, 256);
	put32(data, 8, 56);
	put32(data, 12, 128);
	put32(data, 16, 40);
	put32(data, 20, 17);
	put32(data, 24, 16);
	put32(data, 28, 3);
	put32(data, 32, 64);
	put32(data, 36, 64);
}

int
main(void)
{
	unsigned char data[PROBE_FDT_HEADER_SIZE];
	struct ProbeFdtHeader header;
	valid(data);
	assert(probe_check_fdt_header(data, sizeof(data), &header) == 0);
	assert(header.totalSize == 256 && header.version == 17 && header.bootCpu == 3);
	for (unsigned int size = 0; size < sizeof(data); size++)
		assert(probe_check_fdt_header(data, size, &header) != 0);
	assert(probe_check_fdt_header(0, sizeof(data), &header) != 0);
	assert(probe_check_fdt_header(data, sizeof(data), 0) != 0);
	const ProbeU32 bad[][2] = {
		{0, 0xedfe0dd0}, {4, 39}, {4, PROBE_FDT_MAX_SIZE + 1},
		{20, 16}, {24, 18}, {8, 57}, {16, 41}, {16, 0},
		{8, 0xfffffffcu}, {12, 0xfffffff0u}, {16, 248},
		{32, 0xffffffffu}, {36, 0xffffffffu}, {36, 12},
		{12, 80}, {8, 40}, {12, 40}, {36, 204},
	};
	for (unsigned int index = 0; index < sizeof(bad) / sizeof(bad[0]); index++) {
		valid(data);
		put32(data, bad[index][0], bad[index][1]);
		assert(probe_check_fdt_header(data, sizeof(data), &header) != 0);
	}
	valid(data);
	put32(data, 20, 18);
	put32(data, 24, 17);
	assert(probe_check_fdt_header(data, sizeof(data), &header) == 0);
	put32(data, 32, 0);
	assert(probe_check_fdt_header(data, sizeof(data), &header) == 0);
	assert(probe_pioneer_ram_contains(0x43342000, 46008));
	assert(probe_pioneer_ram_contains(0x200000, 40));
	assert(probe_pioneer_ram_contains(0xbfffffd8, 40));
	assert(!probe_pioneer_ram_contains(0xbfffffd8, 41));
	assert(!probe_pioneer_ram_contains(0xc0000000, 1));
	assert(!probe_pioneer_ram_contains(0xffffffff, 2));
	assert(probe_pioneer_ram_contains(0x100000000ULL, 4096));
	assert(probe_pioneer_ram_contains(0x1fffffffffULL, 1));
	assert(!probe_pioneer_ram_contains(0x1fffffffffULL, 2));
	assert(!probe_pioneer_ram_contains(0x2000000000ULL, 1));
	assert(!probe_pioneer_ram_contains(0x7040000000ULL, 40));
	assert(!probe_pioneer_ram_contains(0, 40));
	assert(!probe_pioneer_ram_contains(0x200000, 0));
	assert(!probe_pioneer_ram_contains(0x200000, ~0ULL));
	assert(!probe_pioneer_ram_contains(~0ULL, 2));
	puts("PASS: FDT header bounds, malformed inputs and physical RAM limits");
	return 0;
}
