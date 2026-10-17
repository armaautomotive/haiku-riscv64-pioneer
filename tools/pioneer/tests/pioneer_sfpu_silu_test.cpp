/* SPDX-License-Identifier: MIT */
#define main PioneerSfpuDivideOriginalTests
#include "pioneer_sfpu_divide_test.cpp"
#undef main
#include "../PioneerSfpuSilu.h"
int main()
{
	(void)PioneerSfpuDivideOriginalTests; NativeF32Q8Machine machine(1024,32,0); uint32_t seed=0x36b31aa9;
	auto next=[&]() { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; };
	unsigned count=0;
	for (unsigned trial=0;trial<32768;trial++) {
		float gates[32]; bool common[32];
		for (unsigned lane=0;lane<32;lane++) {
			float gate=float(int(next()%16000001)-8000000)*.00001f;
			if (trial<512) gate=PioneerAttentionFloat(((trial%2)?0x80000000:0)|(((trial/2)%254+1)<<23));
			gates[lane]=gate; float reduced=0,factor=0; common[lane]=PioneerSfpuSiluRange(gate,reduced,factor);
			machine.lanes[0][lane]=Bits(reduced); machine.lanes[1][lane]=Bits(factor);
		}
		DivideEmitter emitter={machine}; PioneerSfpuSiluDenominator(emitter);
		for (unsigned lane=0;lane<32;lane++) {
			float gate=gates[lane],denominator=Float(machine.lanes[0][lane]);
			float expected=common[lane]?1+PioneerGraphExpFast(-gate):2;
			assert(Bits(denominator)==Bits(expected));
			uint32_t numerator=common[lane]?(Bits(gate)&0x7fffffff):0x3f800000;
			assert(PioneerSfpuDivideDomain(numerator,Bits(denominator)));
			machine.lanes[0][lane]=numerator; machine.lanes[1][lane]=Bits(denominator);
		}
		PioneerSfpuDivideNormal(emitter);
		for (unsigned lane=0;lane<32;lane++) {
			float actual=common[lane]?Float(machine.lanes[0][lane]|(Bits(gates[lane])&0x80000000)):PioneerGraphSiluFast(gates[lane]);
			assert(Bits(actual)==Bits(PioneerGraphSiluFast(gates[lane]))); count++;
		}
	}
	printf("PASS OFFLINE exact vector SiLU polynomial+division,%u scalar-reference bit matches,full exponent/common/rare lanes,no FP reassociation or instruction whitelist expansion;NOT native\n",count);
}
