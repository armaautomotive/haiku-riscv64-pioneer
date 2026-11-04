/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_q4_fpu_block_observer.h"
int main()
{
	uint8_t original[32][144];uint32_t seed=0xb1a038f4;uint64_t count=0;
	auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
	for(unsigned trial=0;trial<32;trial++) {
		for(auto& row:original) {
			for(auto& byte:row)byte=uint8_t(random());
			for(unsigned at:{0U,2U})if((row[at+1]&0x7c)==0x7c)row[at+1]^=4;
		}
		CheckQ4FpuBlock(original,count);
	}
	assert(count==32*32*256);
	printf("PASS OFFBOARD packed-Q4 block binding:%llu original decoded BF16-RNE weights,32 complete two-tile groups,all8 scale/min fields,nibble halves,output/K rows and parities visited exactly once;NO physical UNPACK/Dst/driver/model-quality/speed qualification\n",(unsigned long long)count);
}
