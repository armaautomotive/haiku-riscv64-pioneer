/* SPDX-License-Identifier: MIT */
// Fixed three-head Qwen preparation for one of eight independently owned
// attention groups. Offline candidate: no supplied dimensions/addresses/code.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerGraphNorm.h"
#include "PioneerGraphRope.h"
extern "C" bool attention_prepare_rope_group()
{
	float* query=reinterpret_cast<float*>(0x90000);
	float* key=reinterpret_cast<float*>(0x90400);
	const float* qWeight=reinterpret_cast<const float*>(0x90c00);
	const float* kWeight=reinterpret_cast<const float*>(0x90e00);
	const float* rope=reinterpret_cast<const float*>(0x91000);
	for (unsigned head=0;head<3;head++) {
		float* values=head<2?query+head*128:key;
		const float* weights=head<2?qWeight:kWeight;
		if (!PioneerGraphRmsNorm(128,values,1e-6f,values)) return false;
		for (unsigned i=0;i<128;i++) {
			float value=values[i]*weights[i];
			if ((PioneerAttentionBits(value)&0x7fffffff)>0x477fe000) return false;
			values[i]=value;
		}
		// Exactly the existing checked NEOX arithmetic,one fixed head.
		if (!PioneerGraphRopeHead128(values,rope,values)) return false;
	}
	// Pack only AFTER all128 raw key values have been consumed. The
	// destination lies inside that same independently backed-up span.
	volatile uint32_t* packed=reinterpret_cast<volatile uint32_t*>(0x90400);
	for (unsigned i=0;i<64;i++) packed[i]=PioneerAttentionToHalf(key[i*2])
		|(uint32_t(PioneerAttentionToHalf(key[i*2+1]))<<16);
	asm volatile("fence" ::: "memory");
	return true;
}
