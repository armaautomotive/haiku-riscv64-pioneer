/* SPDX-License-Identifier: MIT */
#include "../../../src/system/boot/platform/riscv/pioneer_kexec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void put32(unsigned char* p, uint32_t value)
{
	unsigned int i;
	for (i = 0; i < 4; i++) p[i] = (unsigned char)(value >> (i * 8));
}

int main(int argc, char** argv)
{
	unsigned char super[512] = {0};
	struct PioneerBfsHeader info;
	assert(PioneerProbeModeValid(1) && PioneerProbeModeValid(2));
	assert(!PioneerProbeModeValid(0) && !PioneerProbeModeValid(3));
	assert(!PioneerProbeModeValid(UINT64_MAX));
	assert(PioneerPreloadPoolEnd(0x1effff5000, 0x1f00000000) == 0xc0000000);
	assert(PioneerPreloadPoolEnd(0xbfff5000, 0xbccc8000) == 0xbccc8000);
	assert(PioneerPreloadPoolEnd(0xbccc7000, 0xbccc8000) == 0xbccc7000);
	assert(PioneerRamRange(0x2200000, 40));
	assert(PioneerRamRange(0x100000000, 0x100000000));
	assert(!PioneerRamRange(0x1000, 40));
	assert(!PioneerRamRange(0xbfffffff, 2));
	assert(!PioneerRamRange(0xc0000000, 1));
	assert(!PioneerRamRange(UINT64_MAX, 2));
	assert(!PioneerRamRange(0x2200000, 0));
	assert(PioneerReadBeCell((const unsigned char*)"\x01\x02\x03\x04", 4) == 0x01020304);
	assert(PioneerReadBeCell(NULL, 4) == UINT64_MAX);
	assert(PioneerReadBeCell(super, 5) == UINT64_MAX);
	put32(super + 32, 0x42465331);
	put32(super + 36, 0x42494745);
	put32(super + 40, 2048);
	put32(super + 44, 11);
	put32(super + 48, 0x200000);
	put32(super + 56, 100);
	put32(super + 64, 2048);
	put32(super + 68, 0xdd121031);
	put32(super + 72, 1);
	put32(super + 76, 14);
	put32(super + 80, 128);
	put32(super + 112, 0x15b6830e);
	put32(super + 116, 1);
	super[120] = 7;
	super[122] = 1;
	assert(PioneerBfsHeaderCheck(super, sizeof(super), 0x100000000, &info));
	assert(info.block_size == 2048 && info.root_offset == (16384ULL + 7) * 2048);
	assert(!PioneerBfsHeaderCheck(super, 131, 0x100000000, &info));
	assert(!PioneerBfsHeaderCheck(NULL, 512, 0x100000000, &info));
	assert(!PioneerBfsHeaderCheck(super, 512, 0x100000000, NULL));
	assert(!PioneerBfsHeaderCheck(super, 512, 0x100000000 - 1, &info));
	put32(super + 116, 128);
	assert(!PioneerBfsHeaderCheck(super, 512, 0x100000000, &info));
	put32(super + 116, 1);
	put32(super + 76, 32);
	assert(!PioneerBfsHeaderCheck(super, 512, 0x100000000, &info));
	put32(super + 76, 14);
	super[122] = 0;
	assert(!PioneerBfsHeaderCheck(super, 512, 0x100000000, &info));
	super[122] = 1;
	put32(super + 40, 4096);
	assert(!PioneerBfsHeaderCheck(super, 512, 0x100000000, &info));
	puts("Haiku kexec RAM/BFS header bounds tests: PASS");
	assert(argc <= 2);
	if (argc == 2) {
		FILE* image = fopen(argv[1], "rb");
		unsigned char root[32];
		assert(image && fseek(image, 0, SEEK_END) == 0);
		long size = ftell(image);
		assert(size > 0 && fseek(image, 512, SEEK_SET) == 0);
		assert(fread(super, 1, 512, image) == 512);
		assert(PioneerBfsHeaderCheck(super, 512, (uint64_t)size, &info));
		assert(fseek(image, (long)info.root_offset, SEEK_SET) == 0);
		assert(fread(root, 1, sizeof(root), image) == sizeof(root));
		assert(PioneerReadLe32(root) == 0x3bbe0ad9);
		assert(PioneerReadLe64(root + 4) == PioneerReadLe64(super + 116));
		assert((PioneerReadLe32(root + 20) & 0xf000) == 0x4000);
		assert(fclose(image) == 0);
		printf("Verified image header/root: %ld bytes, block size %u, root offset %llu\n",
			size, info.block_size, (unsigned long long)info.root_offset);
	}
	return 0;
}
