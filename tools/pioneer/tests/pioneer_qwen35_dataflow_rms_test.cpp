/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#define PIONEER_Q35_OPERATOR_TEST 1
#include "../PioneerQwen35Operators.h"
#include "../PioneerQwen35DataflowRms.h"

using namespace PioneerQwen35;

int main()
{
	float input[Hidden],weight[Hidden],expected[Hidden],actual[Hidden];
	for (unsigned i=0;i<Hidden;i++) {
		const int centered=int(i%61)-30;
		input[i]=float(centered)/64.0f;
		weight[i]=1.0f+float(i%17)/128.0f;
	}
	assert(Rms<Hidden>(input,weight,expected));
	assert(DataflowNormalizeHidden(input,weight,actual));
	assert(memcmp(expected,actual,sizeof(expected))==0);
	assert(DataflowNormalizeHidden(input,weight,input));
	assert(memcmp(expected,input,sizeof(expected))==0);

	uint32_t invalidBits=0x7f800000U;
	memcpy(&input[Hidden/2],&invalidBits,sizeof(invalidBits));
	memset(actual,0x5a,sizeof(actual));
	float before[Hidden];
	memcpy(before,actual,sizeof(before));
	assert(!DataflowNormalizeHidden(input,weight,actual));
	assert(memcmp(before,actual,sizeof(actual))==0);
	std::puts("PASS device Qwen35 hidden RMS matches the pinned operator bitwise");
	return 0;
}
