/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_ACTIVATION_H
#define PIONEER_QWEN38_ACTIVATION_H
#include "PioneerQwen38Hybrid.h"
#include "PioneerGraphMath.h"

// OFFLINE local activation prerequisite for Q4_K/Q6_K dots, NOT a backend,
// native CPU fallback, driver or launch authority. Explicit little-endian
// word layout: F32 scale,256 signed bytes,16 signed16 group sums. Unlike
// Q8_0,the Q8_K scale is F32 and may be NEGATIVE. Never round it to FP16.
// Semantics pinned to llama.cpp c060ca974c77 quantize_row_q8_K_ref:
// first greatest-absolute signed maximum, -127/max, nearest ties EVEN,
// then reciprocal scale. Zero blocks additionally canonicalize ALL sums.
// No fdiv/fcvt/byte-store opcode: exact integer division and bit rounding.
// Caller publishes scratch ONLY after true. Refusals leave it untouched.
struct PioneerQ38ActivationK { uint32_t words[73]; };
static_assert(sizeof(PioneerQ38ActivationK)==292,"exact local Q8_K layout");

static inline int PioneerQ38NearestEven(float value)
{
	uint32_t bits=PioneerAttentionBits(value),magnitude=bits&0x7fffffff;
	if (magnitude<0x3f000000) return 0;
	if (magnitude>=0x43000000) return 1000; // bounded refusal,not saturation
	unsigned shift=150-(magnitude>>23); //17..24 for [.5,128)
	uint32_t mantissa=(magnitude&0x7fffff)|0x800000;
	unsigned integer=mantissa>>shift,remainder=mantissa&((1U<<shift)-1),half=1U<<(shift-1);
	integer+=remainder>half || (remainder==half && (integer&1));
	return bits&0x80000000?-int(integer):int(integer);
}

static inline bool PioneerQ38QuantizeK(const float* input,unsigned count,PioneerQ38ActivationK* output)
{
	if (count!=256 || !PioneerQ38Disjoint(input,256,reinterpret_cast<const float*>(output),73)) return false;
	uint32_t maximum=0,selected=0;
	for (unsigned i=0;i<256;i++) {
		uint32_t bits=PioneerAttentionBits(input[i]),magnitude=bits&0x7fffffff;
		// BabyRISCV flushes source subnormals. Refuse them EXPLICITLY instead
		// of silently claiming equality to the IEEE reference on that input.
		if (magnitude>=0x7f800000 || (magnitude && magnitude<0x00800000)) return false;
		if (magnitude>maximum) { maximum=magnitude; selected=bits; }
	}
	uint32_t scratch[73];
	for (unsigned i=0;i<73;i++) scratch[i]=0;
	if (maximum) {
		float inverse=PioneerGraphDividePositive(127.0f,PioneerAttentionFloat(maximum));
		uint32_t inverseBits=PioneerAttentionBits(inverse);
		if (inverseBits<0x00800000 || inverseBits>=0x7f800000) return false;
		float scale=PioneerGraphDividePositive(1.0f,inverse);
		uint32_t scaleBits=PioneerAttentionBits(scale);
		if (scaleBits<0x00800000 || scaleBits>=0x7f800000) return false;
		uint32_t sign=(selected&0x80000000)^0x80000000;
		inverse=PioneerAttentionFloat(inverseBits|sign); scratch[0]=scaleBits|sign;
		for (unsigned group=0;group<16;group++) {
			int sum=0;
			for (unsigned lane=0;lane<16;lane++) {
				unsigned i=group*16+lane;
				int value=PioneerQ38NearestEven(input[i]*inverse);
				if (value<-127 || value>127) return false;
				scratch[1+i/4]|=(uint32_t(value)&255)<<((i%4)*8); sum+=value;
			}
			// Each sum is in [-2032,2032],represented as signed16 bits.
			scratch[65+group/2]|=(uint32_t(sum)&65535)<<((group%2)*16);
		}
	}
	for (unsigned i=0;i<73;i++) output->words[i]=scratch[i];
	return true;
}

// Q8_0 counterpart for the288 Q8_0 learned matrices. Positive FP16 scale,
// ties AWAY,not Q8_K's signed F32/ties-even convention. Nine local words
// contain the exact34 GGML bytes plus TWO canonical zero padding bytes.
// Word stores avoid any unqualified byte-store instruction. Explicitly
// refuse subnormal source/overflow; a scale rounded to FP16 zero is valid.
struct PioneerQ38Activation0 { uint32_t words[9]; };
static_assert(sizeof(PioneerQ38Activation0)==36,"34 GGML bytes +2 local padding");
static inline bool PioneerQ38Quantize0(const float* input,unsigned count,PioneerQ38Activation0* output)
{
	if (count!=32 || !PioneerQ38Disjoint(input,32,reinterpret_cast<const float*>(output),9)) return false;
	uint32_t maximum=0;
	for (unsigned i=0;i<32;i++) {
		uint32_t magnitude=PioneerAttentionBits(input[i])&0x7fffffff;
		if (magnitude>=0x7f800000 || (magnitude && magnitude<0x00800000)) return false;
		if (magnitude>maximum) maximum=magnitude;
	}
	float scale=maximum?PioneerGraphDividePositive(PioneerAttentionFloat(maximum),127.0f):0;
	float inverse=maximum?PioneerGraphDividePositive(1.0f,scale):0;
	uint16_t half=PioneerAttentionToHalf(scale);
	if ((half&0x7c00)==0x7c00 || !PioneerQ38Finite(inverse)) return false;
	uint32_t scratch[9]; for (unsigned i=0;i<9;i++) scratch[i]=0;
	scratch[0]=half;
	for (unsigned i=0;i<32;i++) {
		int value=PioneerGraphRoundQ8(input[i]*inverse);
		if (value<-127 || value>127) return false;
		unsigned byte=i+2; scratch[byte/4]|=(uint32_t(value)&255)<<((byte%4)*8);
	}
	for (unsigned i=0;i<9;i++) output->words[i]=scratch[i];
	return true;
}
#endif
