/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_BLACKHOLE_BF16_ARITHMETIC_H
#define PIONEER_BLACKHOLE_BF16_ARITHMETIC_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Unqualified Blackhole arithmetic characterization is offboard only
#endif
#include <cstdint>
// Separate CPU diagnostic,NOT a replacement for an accepted hardware oracle.
// Blackhole MVMUL only:BF16 normal/zero inputs,FP32 Dst,stochastic rounding OFF.
// No general IEEE NaN/subnormal,TF32,FP16,integer,SFPU or Wormhole emulation.
// Facts:tt-isa4a73a712,MatrixUnitArithmetic.md. Own integer implementation;
// no vendor code copied. Original observers/Expected()/Work gates unchanged.
struct PioneerBlackholeBf16Arithmetic {
private:
	struct Lane {unsigned exponent;int32_t sum;};
	static uint32_t MagnitudeRound(uint32_t value,unsigned shift) {
		return shift?(value+(UINT32_C(1)<<(shift-1)))>>shift:value;
	}
	static int64_t SignedRound(int64_t value,unsigned shift) {
		if(!shift)return value;
		const int64_t divisor=INT64_C(1)<<shift,biased=value+divisor/2;
		// Explicit floor division;no implementation-defined signed right shift.
		return biased>=0?biased/divisor:-((-biased+divisor-1)/divisor);
	}
	static Lane Products(const uint16_t* a,const uint16_t* b,unsigned phase) {
		unsigned exponents[8],products[8],largest=0;bool negative[8];
		for(unsigned i=0;i<8;i++) {
			const unsigned ae=(a[i]>>7)&255,be=(b[i]>>7)&255;
			const unsigned am=(a[i]&127)<<3,bm=(b[i]&127)<<3;
			const unsigned av=phase&1?(am>>1)&31:16|(am>>6);
			const unsigned bv=phase&2?(bm&15)<<3:64|(bm>>4);
			exponents[i]=ae && be?ae+be:0;products[i]=ae && be?av*bv:0;
			negative[i]=bool((a[i]^b[i])&0x8000);
			if(exponents[i]>largest)largest=exponents[i];
		}
		int32_t sum=0;
		for(unsigned i=0;i<8;i++) {
			const unsigned shift=largest-exponents[i];
			const int32_t term=shift>=16?0:int32_t(MagnitudeRound(products[i],shift));
			sum+=negative[i]?-term:term;
		}
		const unsigned exponent=(largest-127-(phase&1?5:0)-(phase&2?7:0))&1023;
		return exponent && exponent<512?Lane{exponent,sum}:Lane{0,0};
	}
	static bool Operand(uint16_t value) {
		const unsigned exponent=(value>>7)&255;
		return exponent!=255 && (exponent || !(value&127));
	}
public:
	// a/b are complete16-element immutable arrays. Failure leaves result alone.
	// Previous Dst may be normal/zero/infinity (including generated overflow),
	// never NaN/subnormal. Its exponent255 is the documented ordinary exponent,
	// NOT IEEE infinity arithmetic. Positive zero is canonical on every zero.
	static bool Instruction(const uint16_t (&a)[16],const uint16_t (&b)[16],unsigned phase,
		uint32_t previous,uint32_t& result) {
		const unsigned oldExponent=(previous>>23)&255,oldFraction=previous&0x7fffff;
		if(phase>3 || ((!oldExponent || oldExponent==255) && oldFraction))return false;
		for(unsigned i=0;i<16;i++)if(!Operand(a[i]) || !Operand(b[i]))return false;
		const Lane lanes[2]={Products(a,b,phase),Products(a+8,b+8,phase)};
		unsigned common=oldExponent;
		for(const auto& lane:lanes)if(lane.exponent>common)common=lane.exponent;
		int64_t accumulator=0;
		for(const auto& lane:lanes) {
			const unsigned distance=common-lane.exponent,shift=distance>31?31:distance;
			accumulator+=SignedRound(int64_t(lane.sum)*8192,shift);
		}
		const unsigned distance=common-oldExponent,shift=distance>31?31:distance;
		const uint32_t oldMagnitude=oldExponent?(0x800000|oldFraction):0;
		const int64_t aligned=MagnitudeRound(oldMagnitude,shift);
		accumulator+=(previous>>31)?-aligned:aligned;
		uint32_t next=0;
		if(common && accumulator) {
			const uint32_t sign=accumulator<0?0x80000000:0;
			const uint32_t magnitude=uint32_t(accumulator<0?-accumulator:accumulator);
			// The two8-product lanes plus24-bit Dst are bounded below2^29.
			if(magnitude>=UINT32_C(0x20000000))return false;
			unsigned top=0;for(uint32_t scan=magnitude;scan>>=1;)top++;
			int exponent=int(common)+int(top)-23;
			const uint32_t normalized=magnitude<<(28-top);
			uint32_t rounded=(normalized>>5)+((normalized>>4)&1);
			if(rounded&0x1000000){rounded>>=1;exponent++;}
			if(exponent>=255)next=sign|0x7f800000;
			else if(exponent>0)next=sign|(unsigned(exponent)<<23)|(rounded&0x7fffff);
		}
		result=next;return true;
	}
};
#endif
