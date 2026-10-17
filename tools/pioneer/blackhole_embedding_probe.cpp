/* SPDX-License-Identifier: MIT */
// OFFLINE instruction prerequisite ONLY. L1 fixture reads,NO NoC/MMIO,
// no driver image/operation and no device or user-controlled read address.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerDecoderEmbedding.h"
struct EmbeddingFixtureIO {
	unsigned token,reads;
	bool ReadNativeEmbeddingBlock(uint32_t address,uint32_t (&output)[144]) {
		unsigned block=reads>>1;
		if (reads>=64 || address!=0x40000000+(token>>4)*18432+block*576) return false;
		reads++;
		const volatile uint32_t* input=reinterpret_cast<const volatile uint32_t*>(0x90000+block*576);
		asm volatile("fence" ::: "memory");
		for (unsigned i=0;i<144;i++) output[i]=input[i];
		return true;
	}
};
extern "C" unsigned scalar_main()
{
	const volatile uint32_t* job=reinterpret_cast<const volatile uint32_t*>(0x74000);
	if (job[0]!=0x31424d45 || job[1]>=151936 || job[2]!=1024 || job[3]) return 0;
	EmbeddingFixtureIO io={job[1],0};
	if (!PioneerDecoderEmbedding(io,io.token,0x40000000,reinterpret_cast<float*>(0x84000)) || io.reads!=64) return 0;
	asm volatile("fence" ::: "memory"); return 1024;
}
