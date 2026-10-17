/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_ACTIVATION_REFERENCE_H
#define PIONEER_QWEN38_ACTIVATION_REFERENCE_H
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
// Independent host oracle: native IEEE division and nearbyintf,not the
// implementation's integer divider/bit-rounding/word packing. RNE required.
static void Q38ActivationReference(const float* input,uint32_t* words)
{
	float absolute=0,selected=0;
	for (unsigned i=0;i<256;i++) if (fabsf(input[i])>absolute) { absolute=fabsf(input[i]); selected=input[i]; }
	float inverse=absolute?-127.0f/selected:0,scale=absolute?1.0f/inverse:0;
	uint8_t bytes[292]={}; std::memcpy(bytes,&scale,4);
	int sums[16]={};
	for (unsigned i=0;i<256;i++) {
		float rounded=nearbyintf(input[i]*inverse); assert(rounded>=-127 && rounded<=127);
		int value=int(rounded); bytes[4+i]=uint8_t(value&255); sums[i/16]+=value;
	}
	for (unsigned g=0;g<16;g++) { bytes[260+g*2]=uint8_t(sums[g]&255); bytes[261+g*2]=uint8_t((unsigned(sums[g])>>8)&255); }
	std::memcpy(words,bytes,sizeof(bytes));
}
#endif
