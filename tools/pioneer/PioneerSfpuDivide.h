/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_SFPU_DIVIDE_H
#define PIONEER_SFPU_DIVIDE_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// OFFLINE exact32-lane division prerequisite. Positive normal inputs and
// an unrounded quotient exponent1..253 ONLY;no zero/denormal/overflow path.
// Input LReg0=a,1=b;output0=RN(a/b). All8 mutable LRegs are clobbered.
// Uses ONLY already-qualified LOADI/MOV/AND/XOR/SHFT/IADD modifiers;no FP
// MAD/reciprocal approximation,conditional flags,NoC,CSR or supplied code.
// Encodings:vendor SFPIADD/SFPSHFT documentation (Blackhole is explicit
// that IADD is shared with Wormhole;SHFT arithmetic is Blackhole-specific).
static inline bool PioneerSfpuDivideDomain(uint32_t a,uint32_t b)
{
	if (a<0x00800000 || a>=0x7f800000 || b<0x00800000 || b>=0x7f800000) return false;
	int exponent=int(a>>23)-int(b>>23)+127-((a&0x7fffff)<(b&0x7fffff));
	return exponent>=1 && exponent<=253;
}
template<class E> static inline void PioneerSfpuLoad32(E& e,unsigned reg,uint32_t value)
{
	if (value<=65535) e.Op(0x71020000|(reg<<20)|value);
	else { e.Op(0x71000000|(reg<<20)|(value>>16)); e.Op(0x710a0000|(reg<<20)|(value&65535)); }
}
template<class E> static inline void PioneerSfpuMove(E& e,unsigned d,unsigned s) { e.Op(0x7c000000|(s<<8)|(d<<4)); }
template<class E> static inline void PioneerSfpuAdd(E& e,unsigned d,unsigned s) { e.Op(0x79000004|(s<<8)|(d<<4)); }
template<class E> static inline void PioneerSfpuAddI(E& e,unsigned r,int immediate) { e.Op(0x79000005|((unsigned(immediate)&4095)<<12)|(r<<8)|(r<<4)); }
template<class E> static inline void PioneerSfpuShift(E& e,unsigned r,int amount,bool arithmetic=false) { e.Op(0x7a000001|((unsigned(amount)&4095)<<12)|(r<<8)|(r<<4)|(arithmetic?2:0)); }
template<class E> static inline void PioneerSfpuAnd(E& e,unsigned d,unsigned a,unsigned b) { e.Op(0x7e000001|(a<<12)|(b<<8)|(d<<4)); }
template<class E> static inline void PioneerSfpuXor(E& e,unsigned d,unsigned s) { e.Op(0x8d000000|(s<<8)|(d<<4)); }
template<class E> static void PioneerSfpuDivideNormal(E& e)
{
	PioneerSfpuLoad32(e,7,0xffffffff);
	PioneerSfpuMove(e,3,0); PioneerSfpuShift(e,3,-23);
	PioneerSfpuMove(e,4,1); PioneerSfpuShift(e,4,-23); PioneerSfpuXor(e,4,7); PioneerSfpuAddI(e,4,1);
	PioneerSfpuAdd(e,3,4); PioneerSfpuAddI(e,3,127);
	PioneerSfpuLoad32(e,6,0x7fffff); PioneerSfpuAnd(e,0,0,6); PioneerSfpuAnd(e,1,1,6);
	PioneerSfpuLoad32(e,6,0x800000); PioneerSfpuAdd(e,0,6); PioneerSfpuAdd(e,1,6);
	PioneerSfpuMove(e,6,1); PioneerSfpuXor(e,1,7); PioneerSfpuAddI(e,1,1); // 6=mb,1=-mb
	PioneerSfpuMove(e,4,0); PioneerSfpuAdd(e,0,1);
	PioneerSfpuMove(e,5,0); PioneerSfpuShift(e,5,-31,true);
	PioneerSfpuAnd(e,4,4,5); PioneerSfpuAdd(e,0,4); PioneerSfpuAdd(e,3,5); // normalize if ma<mb
	PioneerSfpuLoad32(e,2,1);
	for (unsigned bit=0;bit<23;bit++) {
		PioneerSfpuShift(e,0,1);
		PioneerSfpuMove(e,4,0); PioneerSfpuAdd(e,4,1);
		PioneerSfpuMove(e,5,4); PioneerSfpuShift(e,5,-31,true);
		PioneerSfpuAnd(e,0,6,5); PioneerSfpuAdd(e,0,4); // remainder=tmp+(tmp<0?mb:0)
		PioneerSfpuAddI(e,5,1); // sign mask -1/0 becomes quotient bit0/1
		PioneerSfpuShift(e,2,1); PioneerSfpuAdd(e,2,5);
	}
	// Round up exactly when 2*remainder-mb+(mantissa&1)>0.
	PioneerSfpuMove(e,4,0); PioneerSfpuShift(e,4,1); PioneerSfpuAdd(e,4,1);
	PioneerSfpuLoad32(e,7,1); PioneerSfpuAnd(e,5,2,7); PioneerSfpuAdd(e,4,5);
	PioneerSfpuAddI(e,4,-1); PioneerSfpuShift(e,4,-31,true); PioneerSfpuAddI(e,4,1); PioneerSfpuAdd(e,2,4);
	PioneerSfpuMove(e,4,2); PioneerSfpuShift(e,4,-24); PioneerSfpuAdd(e,3,4);
	PioneerSfpuLoad32(e,7,0x7fffff); PioneerSfpuAnd(e,2,2,7); PioneerSfpuShift(e,3,23); PioneerSfpuAdd(e,2,3);
	PioneerSfpuMove(e,0,2);
}
#endif
