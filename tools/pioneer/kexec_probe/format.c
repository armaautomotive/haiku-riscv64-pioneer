/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "format.h"

_Static_assert(sizeof(ProbeU32) == 4, "32-bit FDT fields required");
_Static_assert(sizeof(ProbeU64) == 8, "64-bit physical addresses required");

static ProbeU32
read_be32(const unsigned char* data)
{
	return (ProbeU32)data[0] << 24 | (ProbeU32)data[1] << 16
		| (ProbeU32)data[2] << 8 | data[3];
}

static int
inside(ProbeU32 offset, ProbeU32 size, ProbeU32 total)
{
	return offset >= PROBE_FDT_HEADER_SIZE && offset <= total
		&& size <= total - offset;
}

static int
overlaps(ProbeU32 offset, ProbeU32 size, ProbeU32 other, ProbeU32 otherSize)
{
	return size != 0 && otherSize != 0
		&& (ProbeU64)offset < (ProbeU64)other + otherSize
		&& (ProbeU64)other < (ProbeU64)offset + size;
}

const char*
probe_check_fdt_header(const unsigned char* data, unsigned long available,
	struct ProbeFdtHeader* result)
{
	if (data == 0 || result == 0 || available < PROBE_FDT_HEADER_SIZE)
		return "short FDT header";
	if (read_be32(data) != 0xd00dfeed)
		return "bad FDT magic";
	ProbeU32 total = read_be32(data + 4);
	ProbeU32 structure = read_be32(data + 8);
	ProbeU32 strings = read_be32(data + 12);
	ProbeU32 reserve = read_be32(data + 16);
	ProbeU32 version = read_be32(data + 20);
	ProbeU32 compatible = read_be32(data + 24);
	ProbeU32 stringsSize = read_be32(data + 32);
	ProbeU32 structureSize = read_be32(data + 36);
	if (total < PROBE_FDT_HEADER_SIZE || total > PROBE_FDT_MAX_SIZE)
		return "FDT size outside probe limit";
	if (version < 17 || compatible > 17 || compatible > version)
		return "unsupported FDT version";
	if ((structure & 3) != 0 || (reserve & 7) != 0)
		return "unaligned FDT section";
	if (structureSize < 16 || (structureSize & 3) != 0
		|| !inside(structure, structureSize, total)
		|| !inside(strings, stringsSize, total) || !inside(reserve, 16, total))
		return "FDT section outside declared bounds";
	if (overlaps(structure, structureSize, strings, stringsSize)
		|| overlaps(reserve, 16, structure, structureSize)
		|| overlaps(reserve, 16, strings, stringsSize))
		return "overlapping FDT sections";
	result->totalSize = total;
	result->version = version;
	result->bootCpu = read_be32(data + 28);
	return 0;
}

int
probe_pioneer_ram_contains(ProbeU64 address, ProbeU64 size)
{
	/* Known board RAM, excluding resident firmware and the 3..4 GiB hole. */
	if (size == 0)
		return 0;
	if (address >= 0x200000 && address < 0xc0000000)
		return size <= 0xc0000000 - address;
	if (address >= 0x100000000ULL && address < 0x2000000000ULL)
		return size <= 0x2000000000ULL - address;
	return 0;
}
