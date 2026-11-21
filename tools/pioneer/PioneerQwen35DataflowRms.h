/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RMS_H
#define PIONEER_QWEN35_DATAFLOW_RMS_H

#include <stdint.h>

#include "PioneerQwen35Geometry.h"
#include "PioneerGdnGateMath.h"
#include "PioneerGraphNorm.h"

namespace PioneerQwen35 {

struct DataflowRmsSum {
	uint32_t high=0,low=0;
	int exponent=0;

	static void RightJam(uint32_t& hi,uint32_t& lo,unsigned shift)
	{
		if (!shift) return;
		if (shift<32) {
			lo=(lo>>shift)|(hi<<(32-shift))|unsigned((lo<<(32-shift))!=0);
			hi>>=shift;
		} else if (shift==32) {
			lo=hi|unsigned(lo!=0);
			hi=0;
		} else if (shift<64) {
			lo=(hi>>(shift-32))|unsigned(lo!=0 || (hi<<(64-shift))!=0);
			hi=0;
		} else {
			lo=unsigned((hi|lo)!=0);
			hi=0;
		}
	}

	bool Add(float term)
	{
		const uint32_t bits=PioneerAttentionBits(term);
		if ((bits&0x80000000U) || bits>=0x7f800000U
			|| (bits && bits<0x00800000U)) return false;
		if (!bits) return true;
		const unsigned mantissa=(bits&0x7fffffU)|0x800000U;
		const int e=int(bits>>23)-127;
		if (!(high|low)) {
			high=mantissa>>3;
			low=mantissa<<29;
			exponent=e;
			return true;
		}
		uint32_t ah=(high<<3)|(low>>29),al=low<<3,bh=mantissa,bl=0;
		if (e>exponent) { RightJam(ah,al,unsigned(e-exponent)); exponent=e; }
		else RightJam(bh,bl,unsigned(exponent-e));
		const uint32_t old=al;
		al+=bl;
		ah+=bh+unsigned(al<old);
		if (ah&0x01000000U) { RightJam(ah,al,1); exponent++; }
		const unsigned round=al&7;
		low=(al>>3)|(ah<<29);
		high=ah>>3;
		if (round>4 || (round==4 && (low&1))) { if (++low==0) high++; }
		if (high==0x00200000U) {
			low=(low>>1)|(high<<31);
			high>>=1;
			exponent++;
		}
		return true;
	}

	bool Mean5120(float& mean) const
	{
		if (!(high|low)) { mean=0; return true; }
		const unsigned shift=high>=0x00140000U?2:3;
		const uint32_t nh=(high<<shift)|(low>>(32-shift)),nl=low<<shift;
		uint32_t qh=0,ql=0,remainder=0;
		for (int bit=55;bit>=0;bit--) {
			remainder=(remainder<<1)|((bit>=32?nh>>unsigned(bit-32)
				:nl>>unsigned(bit))&1);
			const unsigned digit=remainder>=5;
			remainder-=digit*5;
			qh=(qh<<1)|(ql>>31);
			ql=(ql<<1)|digit;
		}
		if (remainder*2>5) { if (++ql==0) qh++; }
		int e=exponent-int(shift)-10;
		if (qh==0x00200000U) { ql=(ql>>1)|(qh<<31); qh>>=1; e++; }
		if (e< -126 || e>127 || qh<0x00100000U || qh>=0x00200000U)
			return false;
		uint32_t mantissa=(qh<<3)|(ql>>29);
		const uint32_t tail=ql&0x1fffffffU;
		if (tail>0x10000000U || (tail==0x10000000U && (mantissa&1)))
			mantissa++;
		if (mantissa==0x01000000U) { mantissa>>=1; e++; }
		if (e>127) return false;
		mean=PioneerAttentionFloat((uint32_t(e+127)<<23)|(mantissa&0x7fffffU));
		return true;
	}
};

inline bool DataflowRmsDomain(float value)
{
	const uint32_t magnitude=PioneerAttentionBits(value)&0x7fffffffU;
	return magnitude<=0x477fe000U && (!magnitude || magnitude>=0x26800000U);
}

inline bool DataflowHiddenRmsScale(const float* input,float& scale)
{
	if (!input || (reinterpret_cast<uintptr_t>(input)&3)) return false;
	DataflowRmsSum sum;
	for (unsigned i=0;i<Hidden;i++) {
		if (!DataflowRmsDomain(input[i])
			|| !sum.Add(PioneerQ38PrepareFlush(input[i]*input[i]))) return false;
	}
	float mean=0;
	if (!sum.Mean5120(mean)) return false;
	mean*=2.0f;
	const float denominator=PioneerGraphSqrtPositive(mean+1e-6f);
	if (!(denominator>0)
		|| (PioneerAttentionBits(denominator)&0x7f800000U)==0x7f800000U)
		return false;
	const float result=PioneerGraphDividePositive(1.0f,denominator);
	if (!(result>0) || !DataflowRmsDomain(result)) return false;
	scale=result;
	return true;
}

// Exact hidden-width RMS arithmetic for the device worker. The reduction
// follows the pinned operator's rounded-square, software-binary64 sum, sqrt,
// divide, and two rounded output multiplies. This helper executes in the
// device firmware; it is not a host inference path.
inline bool DataflowNormalizeHidden(const float* input,const float* weight,
	float* output)
{
	if (!input || !weight || !output
		|| (reinterpret_cast<uintptr_t>(input)&3)
		|| (reinterpret_cast<uintptr_t>(weight)&3)
		|| (reinterpret_cast<uintptr_t>(output)&3)) return false;
	for (unsigned i=0;i<Hidden;i++) if (!DataflowRmsDomain(weight[i])) return false;
	float scale=0;
	if (!DataflowHiddenRmsScale(input,scale)) return false;
	for (unsigned i=0;i<Hidden;i++) {
		const float value=(input[i]*scale)*weight[i];
		if ((PioneerAttentionBits(value)&0x7f800000U)==0x7f800000U) return false;
		output[i]=value;
	}
	return true;
}

} // namespace PioneerQwen35

#endif
