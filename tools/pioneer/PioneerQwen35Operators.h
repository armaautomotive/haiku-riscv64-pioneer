/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_OPERATORS_H
#define PIONEER_QWEN35_OPERATORS_H

#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) \
	|| (!defined(PIONEER_Q35_OPERATOR_TEST) \
		&& !(defined(PIONEER_ATTENTION_FREESTANDING) && defined(__riscv_xlen) && __riscv_xlen==32))
#error The 4B operators are for the RV32 card controller or the explicit offline tests
#endif

#include "PioneerGdnGateMath.h"
#include "PioneerGraphFma.h"
#include "PioneerQ38LearnedRms.h"
#include "PioneerQwen35Geometry.h"

namespace PioneerQwen35 {

constexpr float Epsilon=1e-6f;
constexpr float DeltaQueryScale=0.0883883461356163f; // 1/sqrt(128)

inline bool Finite(float x)
{
	return (PioneerAttentionBits(x)&0x7f800000)!=0x7f800000;
}

inline bool Finite(const float* values, unsigned count)
{
	for (unsigned i=0; i<count; i++)
		if (!Finite(values[i])) return false;
	return true;
}

inline float Sigmoid(float x)
{
	float magnitude=PioneerAttentionFloat(PioneerAttentionBits(x)&0x7fffffff);
	float e=PioneerQ38PrepareNegativeExp(-magnitude);
	return PioneerGraphDividePositive(x>=0?1.0f:e,1.0f+e);
}

inline bool SumRoundedSquares(const float* input, unsigned count, PioneerQ38RmsSum& sum)
{
	for (unsigned i=0; i<count; i++) {
		float term=PioneerQ38PrepareFlush(input[i]*input[i]);
		if (!sum.Add(term)) return false;
	}
	return true;
}

// Convert the software binary64 accumulator after scaling by a power of two.
inline float RoundedSum(const PioneerQ38RmsSum& sum, int power)
{
	if (!(sum.high|sum.low)) return 0;
	unsigned mantissa=(sum.high<<3)|(sum.low>>29);
	unsigned tail=sum.low&0x1fffffff;
	int exponent=sum.exponent+power;
	if (tail>0x10000000 || (tail==0x10000000 && (mantissa&1))) mantissa++;
	if (mantissa==0x01000000) { mantissa>>=1; exponent++; }
	return exponent>127 ? PioneerAttentionFloat(0x7f800000)
		: exponent < -126 ? 0
		: PioneerAttentionFloat((unsigned(exponent+127)<<23)|(mantissa&0x7fffff));
}

template<unsigned Count>
inline bool Rms(const float* input, const float* weight, float* output)
{
	static_assert(Count==Hidden || Count==DenseHeadWidth || Count==DeltaWidth, "4B RMS width");
	(void)&PioneerQ38LearnedRms;
	PioneerQ38RmsSum sum;
	if (!SumRoundedSquares(input,Count,sum) || !Finite(weight,Count)) return false;
	float mean;
	if (Count==Hidden) {
		// Halving the divisor scales the rounded quotient by exactly two
		// over the normal domain of the shared positive accumulator.
		if (!sum.Mean5120(mean)) return false;
		mean*=2;
	} else
		mean=RoundedSum(sum,Count==DenseHeadWidth?-8:-7);
	float denominator=PioneerGraphSqrtPositive(mean+Epsilon);
	if (!(denominator>0) || !Finite(denominator)) return false;
	float scale=PioneerGraphDividePositive(1.0f,denominator);
	for (unsigned i=0; i<Count; i++) {
		float value=(input[i]*scale)*weight[i];
		if (!Finite(value)) return false;
		output[i]=value;
	}
	return true;
}

inline bool NormalizeDelta(float* vector)
{
	PioneerQ38RmsSum sum;
	if (!SumRoundedSquares(vector,DeltaWidth,sum)) return false;
	float denominator=PioneerGraphSqrtPositive(RoundedSum(sum,0));
	if (!Finite(denominator)) return false;
	if (denominator<Epsilon) denominator=Epsilon;
	float scale=PioneerGraphDividePositive(1.0f,denominator);
	for (unsigned i=0; i<DeltaWidth; i++) vector[i]*=scale;
	return Finite(vector,DeltaWidth);
}

// Conv4 weights and history are channel-major. Runs on a local 16-channel
// window with separate current and next history supplied by the ROOT.
inline bool Conv16(float* input, const float* weight, const float* previous, float* next)
{
	for (unsigned c=0; c<16; c++) {
		float sum=input[c]*weight[c*4+3];
		float first=previous[c*3]*weight[c*4];
		float second=previous[c*3+1]*weight[c*4+1];
		float third=previous[c*3+2]*weight[c*4+2];
		sum=((first+second)+third)+sum;
		if (!Finite(sum)) return false;
		next[c*3]=previous[c*3+1];
		next[c*3+1]=previous[c*3+2];
		next[c*3+2]=input[c];
		input[c]=sum*Sigmoid(sum);
		if (!Finite(input[c])) return false;
	}
	return true;
}

// State is [value lane][key lane] as in the current GGML fused delta net.
// Decay and beta are per value head, with Q/K shared modulo 16 heads.
inline bool DeltaHead(const float* query, const float* key, const float* value,
	float alpha, float betaLogit, float negativeA, float bias, float* state, float* output)
{
	if (!Finite(alpha) || !Finite(betaLogit) || !Finite(negativeA) || negativeA>=0
		|| !Finite(bias) || !Finite(alpha+bias)) return false;
	float decay=PioneerQ38PrepareNegativeExp(negativeA*PioneerQ38PrepareSoftplus(alpha+bias));
	float beta=Sigmoid(betaLogit);
	if (!Finite(decay) || !Finite(beta)) return false;
	for (unsigned v=0; v<DeltaWidth; v++) {
		float* row=state+v*DeltaWidth;
		float prediction=0;
		for (unsigned k=0; k<DeltaWidth; k++) {
			row[k]*=decay;
			prediction+=row[k]*key[k];
		}
		float delta=(value[v]-prediction)*beta;
		float result=0;
		for (unsigned k=0; k<DeltaWidth; k++) {
			row[k]+=key[k]*delta;
			result+=row[k]*query[k];
		}
		if (!Finite(result)) return false;
		output[v]=result*DeltaQueryScale;
	}
	return true;
}

inline bool Rotate(float* head, const float* rope)
{
	for (unsigned i=0; i<Rotary/2; i++) {
		float a=head[i], b=head[i+Rotary/2], c=rope[2*i], s=rope[2*i+1];
		head[i]=PioneerGraphFma(a,c,-b*s);
		head[i+Rotary/2]=PioneerGraphFma(a,s,b*c);
	}
	return Finite(head,Rotary);
}

inline bool PackHead(const float* head, uint32_t* packed)
{
	for (unsigned i=0; i<DenseHeadWidth/2; i++) {
		unsigned lo=PioneerAttentionToHalf(head[i*2]);
		unsigned hi=PioneerAttentionToHalf(head[i*2+1]);
		if ((lo&0x7c00)==0x7c00 || (hi&0x7c00)==0x7c00) return false;
		packed[i]=lo|(hi<<16);
	}
	return true;
}

inline float UnpackLane(const uint32_t* packed, unsigned lane)
{
	return PioneerAttentionHalf(uint16_t(packed[lane/2]>>((lane&1)*16)));
}

// Query-independent masking and stable one-pass softmax over consecutive
// positions in a single K/V head. Only the final gated row is exported.
struct AttentionRow {
	float* sums;
	float maximum=0, denominator=0;

	bool Add(const float* query, const uint32_t* key, const uint32_t* value)
	{
		float score=0;
		for (unsigned i=0; i<DenseHeadWidth; i++) score+=query[i]*UnpackLane(key,i);
		score*=0.0625f;
		if (!Finite(score)) return false;
		float old=0, current=1;
		if (denominator==0)
			maximum=score;
		else if (score>maximum) {
			old=PioneerQ38PrepareNegativeExp(maximum-score);
			maximum=score;
		} else {
			old=1;
			current=PioneerQ38PrepareNegativeExp(score-maximum);
		}
		for (unsigned i=0; i<DenseHeadWidth; i++) {
			float lane=UnpackLane(value,i);
			if (!Finite(lane)) return false;
			sums[i]=(denominator==0?0.0f:old*sums[i])+current*lane;
		}
		denominator=old*denominator+current;
		return denominator>0 && Finite(denominator);
	}

	bool Finish(const float* gate)
	{
		if (!(denominator>0) || !Finite(gate,DenseHeadWidth)) return false;
		float inverse=PioneerGraphDividePositive(1.0f,denominator);
		for (unsigned i=0; i<DenseHeadWidth; i++) sums[i]=(sums[i]*inverse)*Sigmoid(gate[i]);
		return Finite(sums,DenseHeadWidth);
	}
};
}
#endif
