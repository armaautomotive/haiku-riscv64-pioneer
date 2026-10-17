/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_ACTIVATION_REFERENCE_H
#define PIONEER_QWEN38_ACTIVATION_REFERENCE_H
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
// Independent host oracle: native IEEE division and nearbyintf,not the
// implementation's integer divider/bit-rounding/word packing. RNE required.
static inline void Q38ActivationReference(const float* input,uint32_t* words)
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
static inline void Q38Activation0Reference(const float* input,uint32_t* words)
{
	float maximum=0;
	for (unsigned i=0;i<32;i++) maximum=fmaxf(maximum,fabsf(input[i]));
	float scale=maximum/127.0f,inverse=scale?1.0f/scale:0;
	// Host hardware binary16 conversion is independent of card bit code.
	_Float16 half=static_cast<_Float16>(scale); static_assert(sizeof(half)==2,"binary16 host oracle");
	uint8_t bytes[36]={}; std::memcpy(bytes,&half,2);
	for (unsigned i=0;i<32;i++) { float q=roundf(input[i]*inverse); assert(q>=-127 && q<=127); bytes[2+i]=uint8_t(int(q)&255); }
	std::memcpy(words,bytes,sizeof(bytes));
}
#endif
