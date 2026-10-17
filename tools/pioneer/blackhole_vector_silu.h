/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_VECTOR_SILU_H
#define BLACKHOLE_VECTOR_SILU_H
#include "PioneerSfpuSilu.h"
// GPU-only helper;requires the already-owned initialized attention SFPU.
// Existing frozen gate/up L1 inputs;only private Dst0..11 and RISC stack.
struct GroupSiluEmitter { void Op(uint32_t instruction) { Emit(instruction); } };
#if PIONEER_ATTENTION_PARALLEL_ROPE
static __attribute__((section(".rope_ext"),noinline)) void GroupVectorSilu(const float* gate,const float* up,float* values)
#else
static void GroupVectorSilu(const float* gate,const float* up,float* values)
#endif
{
	float first[32],second[32];
	for (unsigned i=0;i<32;i++) {
		PioneerSfpuSiluRange(gate[i],first[i],second[i]);
		Words(0xffbd8000)[LaneOffset(i)]=PioneerAttentionBits(first[i]);
		Words(0xffbd8100)[LaneOffset(i)]=PioneerAttentionBits(second[i]);
	}
	Fence();
	for (unsigned i=0;i<32;i++) if (Words(0xffbd8000)[LaneOffset(i)]!=PioneerAttentionBits(first[i])
		|| Words(0xffbd8100)[LaneOffset(i)]!=PioneerAttentionBits(second[i])) attention_fault_park();
	Emit(0x70030000); Emit(0x70130004);
	GroupSiluEmitter emitter; PioneerSfpuSiluDenominator(emitter);
	Emit(0x72030008); Drain();
	for (unsigned i=0;i<32;i++) {
		unsigned absolute=PioneerAttentionBits(gate[i])&0x7fffffff;
		uint32_t a=absolute>=0x0d800000 && absolute<0x42a00000?absolute:0x3f800000;
		uint32_t b=Words(0xffbd8200)[LaneOffset(i)];
		if (!PioneerSfpuDivideDomain(a,b)) attention_fault_park();
		first[i]=PioneerAttentionFloat(a); second[i]=PioneerAttentionFloat(b);
		Words(0xffbd8000)[LaneOffset(i)]=a; Words(0xffbd8100)[LaneOffset(i)]=b;
	}
	Fence();
	for (unsigned i=0;i<32;i++) if (Words(0xffbd8000)[LaneOffset(i)]!=PioneerAttentionBits(first[i])
		|| Words(0xffbd8100)[LaneOffset(i)]!=PioneerAttentionBits(second[i])) attention_fault_park();
	Emit(0x70030000); Emit(0x70130004); PioneerSfpuDivideNormal(emitter);
	Emit(0x72030008); Drain();
	for (unsigned i=0;i<32;i++) {
		uint32_t bits=PioneerAttentionBits(gate[i]),absolute=bits&0x7fffffff;
		float value;
		if (absolute>=0x0d800000 && absolute<0x42a00000) {
			uint32_t result=Words(0xffbd8200)[LaneOffset(i)];
			if (result<0x00800000 || result>=0x7f800000) attention_fault_park();
			value=PioneerAttentionFloat(result|(bits&0x80000000));
		} else value=PioneerGraphSiluFast(gate[i]); // same bounded CARD-side corner arithmetic
		values[i]=value*up[i];
	}
}
#endif
