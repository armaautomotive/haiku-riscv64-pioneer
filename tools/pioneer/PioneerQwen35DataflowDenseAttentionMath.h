/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_MATH_H
#define PIONEER_QWEN35_DATAFLOW_DENSE_ATTENTION_MATH_H

#include "PioneerQwen35DataflowRms.h"
#include "PioneerGraphFma.h"

namespace PioneerQwen35 {

inline bool DataflowDenseFinite(float value)
{
	return (PioneerAttentionBits(value)&0x7f800000U)!=0x7f800000U;
}

inline bool DataflowDenseRoundedMean(const DataflowRmsSum& sum,float& mean)
{
	if (!(sum.high|sum.low)) {
		mean=0;
		return true;
	}
	uint32_t mantissa=(sum.high<<3)|(sum.low>>29);
	const uint32_t tail=sum.low&0x1fffffffU;
	int exponent=sum.exponent-8;
	if (tail>0x10000000U || (tail==0x10000000U && (mantissa&1))) mantissa++;
	if (mantissa==0x01000000U) { mantissa>>=1; exponent++; }
	if (exponent>127) return false;
	mean=exponent< -126?0.0f:PioneerAttentionFloat(
		(uint32_t(exponent+127)<<23)|(mantissa&0x7fffffU));
	return true;
}

// Device-side Q/K RMS for one dense head; it matches the pinned 256-wide
// operator's rounded-square sum and output multiply order.
inline bool DataflowDenseNormalize(const float* input,const float* weight,
	float* output)
{
	if (!input || !weight || !output) return false;
	DataflowRmsSum sum;
	for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
		if (!DataflowRmsDomain(input[lane]) || !DataflowRmsDomain(weight[lane])
			|| !sum.Add(PioneerQ38PrepareFlush(input[lane]*input[lane]))) return false;
	}
	float mean=0;
	if (!DataflowDenseRoundedMean(sum,mean)) return false;
	const float denominator=PioneerGraphSqrtPositive(mean+1e-6f);
	if (!(denominator>0) || !DataflowDenseFinite(denominator)) return false;
	const float scale=PioneerGraphDividePositive(1.0f,denominator);
	if (!(scale>0) || !DataflowRmsDomain(scale)) return false;
	for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
		const float value=(input[lane]*scale)*weight[lane];
		if (!DataflowDenseFinite(value)) return false;
		output[lane]=value;
	}
	return true;
}

inline bool DataflowDenseRotate(float* head,const float* rope)
{
	if (!head || !rope) return false;
	for (unsigned pair=0;pair<Rotary/2;pair++) {
		const float a=head[pair],b=head[pair+Rotary/2];
		const float cosine=rope[2*pair],sine=rope[2*pair+1];
		head[pair]=PioneerGraphFma(a,cosine,-b*sine);
		head[pair+Rotary/2]=PioneerGraphFma(a,sine,b*cosine);
	}
	for (unsigned lane=0;lane<Rotary;lane++)
		if (!DataflowDenseFinite(head[lane])) return false;
	return true;
}

inline bool DataflowDensePackHead(const float* head,uint32_t* packed)
{
	if (!head || !packed) return false;
	for (unsigned pair=0;pair<DenseHeadWidth/2;pair++) {
		const unsigned low=PioneerAttentionToHalf(head[pair*2]);
		const unsigned high=PioneerAttentionToHalf(head[pair*2+1]);
		if ((low&0x7c00U)==0x7c00U || (high&0x7c00U)==0x7c00U)
			return false;
		packed[pair]=low|(high<<16);
	}
	return true;
}

inline float DataflowDenseUnpackLane(const uint32_t* packed,unsigned lane)
{
	return PioneerAttentionHalf(uint16_t(packed[lane/2]>>((lane&1)*16)));
}

struct DataflowDenseAttentionRow {
	float* sums;
	float maximum=0,denominator=0;

	bool Add(const float* query,const uint32_t* key,const uint32_t* value)
	{
		if (!sums || !query || !key || !value) return false;
		float score=0;
		for (unsigned lane=0;lane<DenseHeadWidth;lane++)
			score+=query[lane]*DataflowDenseUnpackLane(key,lane);
		score*=0.0625f;
		if (!DataflowDenseFinite(score)) return false;
		float old=0,current=1;
		if (denominator==0)
			maximum=score;
		else if (score>maximum) {
			old=PioneerQ38PrepareNegativeExp(maximum-score);
			maximum=score;
		} else {
			old=1;
			current=PioneerQ38PrepareNegativeExp(score-maximum);
		}
		for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
			const float element=DataflowDenseUnpackLane(value,lane);
			if (!DataflowDenseFinite(element)) return false;
			sums[lane]=(denominator==0?0.0f:old*sums[lane])+current*element;
		}
		denominator=old*denominator+current;
		return denominator>0 && DataflowDenseFinite(denominator);
	}

	bool Finish(const float* gate)
	{
		if (!sums || !gate || !(denominator>0)
			|| !DataflowDenseFinite(denominator)) return false;
		for (unsigned lane=0;lane<DenseHeadWidth;lane++)
			if (!DataflowDenseFinite(gate[lane])) return false;
		const float inverse=PioneerGraphDividePositive(1.0f,denominator);
		for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
			const float magnitude=PioneerAttentionFloat(
				PioneerAttentionBits(gate[lane])&0x7fffffffU);
			const float exponential=PioneerQ38PrepareNegativeExp(-magnitude);
			const float sigmoid=PioneerGraphDividePositive(
				gate[lane]>=0?1.0f:exponential,1.0f+exponential);
			sums[lane]=(sums[lane]*inverse)*sigmoid;
			if (!DataflowDenseFinite(sums[lane])) return false;
		}
		return true;
	}
};

} // namespace PioneerQwen35

#endif
