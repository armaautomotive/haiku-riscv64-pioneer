/* SPDX-License-Identifier: MIT */
#ifndef HAIKU_PIONEER_KEXEC_H
#define HAIKU_PIONEER_KEXEC_H
#include <stddef.h>
#include <stdint.h>

struct PioneerBfsHeader {
	uint32_t block_size;
	uint64_t num_blocks;
	uint64_t root_offset;
};

static inline int PioneerProbeModeValid(uint64_t mode)
{
	return mode == 1 || mode == 2; /* Neither mode may execute a kernel. */
}

static inline uint64_t PioneerPreloadPoolEnd(uint64_t fdt, uint64_t initrd)
{
	uint64_t end = 0xc0000000;
	if (fdt >= 0x2200000 && fdt < end) end = fdt;
	if (initrd >= 0x2200000 && initrd < end) end = initrd;
	return end;
}

static inline uint32_t PioneerReadLe32(const unsigned char* p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16
		| (uint32_t)p[3] << 24;
}

static inline uint64_t PioneerReadLe64(const unsigned char* p)
{
	return (uint64_t)PioneerReadLe32(p) | (uint64_t)PioneerReadLe32(p + 4) << 32;
}

static inline uint64_t PioneerReadBeCell(const unsigned char* p, size_t length)
{
	uint64_t value = 0;
	size_t i;
	if (!p || (length != 4 && length != 8))
		return UINT64_MAX;
	for (i = 0; i < length; i++)
		value = value << 8 | p[i];
	return value;
}

static inline int PioneerRamRange(uint64_t start, uint64_t size)
{
	if (!size || size > UINT64_MAX - start)
		return 0;
	return (start >= 0x2200000 && start + size <= 0xc0000000)
		|| (start >= 0x100000000 && start + size <= 0x2000000000);
}

/* Superblock at byte 512 of a raw BFS partition. No root-data read here.
 * Offsets follow disk_super_block in the production BFS format header.
 * This is a bounds/header check, not journal replay or a filesystem audit.
 */
static inline int PioneerBfsHeaderCheck(const unsigned char* super,
	size_t available, uint64_t disk_size, struct PioneerBfsHeader* info)
{
	uint32_t block, shift, ag_shift, groups, per_group, root_group, root_start;
	uint32_t root_length;
	uint64_t blocks, root_block;
	if (!super || !info || available < 132 || disk_size < 1024
		|| disk_size > 8ULL * 1024 * 1024 * 1024)
		return 0;
	if (PioneerReadLe32(super + 32) != 0x42465331
		|| PioneerReadLe32(super + 36) != 0x42494745
		|| PioneerReadLe32(super + 68) != 0xdd121031
		|| PioneerReadLe32(super + 112) != 0x15b6830e)
		return 0;
	block = PioneerReadLe32(super + 40);
	shift = PioneerReadLe32(super + 44);
	blocks = PioneerReadLe64(super + 48);
	if (shift < 9 || shift > 13 || block != (1U << shift)
		|| !blocks || blocks > disk_size / block
		|| PioneerReadLe64(super + 56) > blocks
		|| PioneerReadLe32(super + 64) != block)
		return 0;
	per_group = PioneerReadLe32(super + 72);
	ag_shift = PioneerReadLe32(super + 76);
	groups = PioneerReadLe32(super + 80);
	root_group = PioneerReadLe32(super + 116);
	root_start = (uint32_t)super[120] | (uint32_t)super[121] << 8;
	root_length = (uint32_t)super[122] | (uint32_t)super[123] << 8;
	/* blocks_per_ag counts bitmap blocks, not data blocks in the group. */
	if (ag_shift < 1 || ag_shift > 31 || !groups || !per_group
		|| (uint64_t)per_group * block * 8 != (1ULL << ag_shift)
		|| groups != (blocks + (1ULL << ag_shift) - 1) / (1ULL << ag_shift)
		|| root_group >= groups || !root_length
		|| root_start >= (1ULL << ag_shift)
		|| root_length > (1ULL << ag_shift) - root_start)
		return 0;
	root_block = ((uint64_t)root_group << ag_shift) + root_start;
	if (root_block >= blocks || root_length > blocks - root_block)
		return 0;
	info->block_size = block;
	info->num_blocks = blocks;
	info->root_offset = root_block * block;
	return 1;
}

#if defined(PIONEER_KEXEC_PROBE) && defined(__cplusplus)
bool PioneerKexecPreloadActive();
const void* PioneerKexecRamDisk(size_t& size);
void PioneerKexecWrite(const char* data, size_t size);
void PioneerKexecStop(bool success, const char* reason) __attribute__((noreturn));
extern "C" void pioneer_kexec_preload(uint64_t hart, const void* fdt,
	uint64_t start, uint64_t size) __attribute__((noreturn));
#endif
#endif
