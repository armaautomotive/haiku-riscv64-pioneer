/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GRAPH_FMA_H
#define PIONEER_GRAPH_FMA_H
#include "PioneerAttentionMath.h"
// Exact binary32 RNE multiply-add using integer limbs. BRISC FMADD is not
// qualified, and its ordinary float multiply flushes subnormals. Keep the
// full 48-bit product and enough aligned guard bits for cancellation.
typedef __UINT64_TYPE__ PioneerGraphU64;
static inline PioneerGraphU64 PioneerGraphRight(PioneerGraphU64 value,unsigned shift)
{
	uint32_t low=uint32_t(value),high=uint32_t(value>>32);
	if (!shift) return value;
	if (shift>=64) return 0;
	if (shift>=32) return high>>(shift-32);
	return (PioneerGraphU64(high>>shift)<<32)|((low>>shift)|(high<<(32-shift)));
}
static inline PioneerGraphU64 PioneerGraphLeft(PioneerGraphU64 value,unsigned shift)
{
	uint32_t low=uint32_t(value),high=uint32_t(value>>32);
	if (!shift) return value;
	if (shift>=64) return 0;
	if (shift>=32) return PioneerGraphU64(low<<(shift-32))<<32;
	return (PioneerGraphU64((high<<shift)|(low>>(32-shift)))<<32)|(low<<shift);
}
static inline PioneerGraphU64 PioneerGraphShiftJam(PioneerGraphU64 value,unsigned shift)
{
	if (!shift) return value;
	if (shift>=64) return value!=0;
	PioneerGraphU64 result=PioneerGraphRight(value,shift);
	return result|(PioneerGraphLeft(result,shift)!=value);
}
// A fixed-depth normalizer avoids walking up to61 bits in every exact
// fallback. Only qualified low-word shifts/comparisons; no CLZ/MULHU ISA
// expansion. The caller has already excluded zero. Keep legacy images
// byte-identical unless this separately tested build option is selected.
static inline unsigned PioneerGraphHighest(PioneerGraphU64 value)
{
	uint32_t high=uint32_t(value>>32),word=high?high:uint32_t(value);
	unsigned bit=high?32:0;
	if (word>=0x10000) { word>>=16; bit+=16; }
	if (word>=0x100) { word>>=8; bit+=8; }
	if (word>=0x10) { word>>=4; bit+=4; }
	if (word>=4) { word>>=2; bit+=2; }
	return bit+(word>=2);
}
static inline float PioneerGraphFma(float a,float b,float c)
{
	uint32_t aa=PioneerAttentionBits(a),bb=PioneerAttentionBits(b),cc=PioneerAttentionBits(c);
	uint32_t am=aa&0x7fffffff,bm=bb&0x7fffffff,cm=cc&0x7fffffff;
	if (am>=0x7f800000 || bm>=0x7f800000 || cm>=0x7f800000) return PioneerAttentionFloat(0x7fc00000);
	uint32_t ps=(aa^bb)&0x80000000,cs=cc&0x80000000;
	if (!am || !bm) return cm?c:PioneerAttentionFloat(ps&cs);
	int ae=int(am>>23)-127,be=int(bm>>23)-127,ce=int(cm>>23)-127;
	uint32_t ma=am&0x7fffff,mb=bm&0x7fffff,mc=cm&0x7fffff;
	if (am>>23) ma|=0x800000; else { ae=-126; while (!(ma&0x800000)) { ma<<=1; ae--; } }
	if (bm>>23) mb|=0x800000; else { be=-126; while (!(mb&0x800000)) { mb<<=1; be--; } }
	if (cm>>23) mc|=0x800000; else if (mc) { ce=-126; while (!(mc&0x800000)) { mc<<=1; ce--; } }
	// A normalized operand starts at bit60; the product may also use bit61.
	// Four 12-bit products use only the already-qualified low-word MUL;
	// do not introduce MULHU merely to assemble the 48-bit significand.
	uint32_t p0=(ma&4095)*(mb&4095),p1=(ma>>12)*(mb&4095)+(ma&4095)*(mb>>12)+(p0>>12);
	uint32_t p2=(ma>>12)*(mb>>12)+(p1>>12);
	PioneerGraphU64 product=((PioneerGraphU64(p2)<<24)|((p1&4095)<<12)|(p0&4095))<<14,addend=PioneerGraphU64(mc)<<37;
	int exponent=ae+be;
	if (mc && ce>exponent) { product=PioneerGraphShiftJam(product,unsigned(ce-exponent)); exponent=ce; }
	else if (mc) addend=PioneerGraphShiftJam(addend,unsigned(exponent-ce));
	PioneerGraphU64 result;
	if (ps==cs) result=product+addend;
	else if (product>=addend) result=product-addend;
	else { result=addend-product; ps=cs; }
	if (!result) return 0; // Exact cancellation is +0 under RNE.
#if PIONEER_GRAPH_FMA_FIXED_NORMALIZE
	unsigned highest=PioneerGraphHighest(result);
#else
	unsigned highest=0; for (PioneerGraphU64 v=result;v>>1;v>>=1) highest++;
#endif
	int e=exponent+int(highest)-60;
	int shift=e< -126?-exponent-89:int(highest)-23;
	PioneerGraphU64 rounded;
	if (shift>=64) rounded=0;
	else if (shift>0) {
		rounded=PioneerGraphRight(result,unsigned(shift));
		PioneerGraphU64 remainder=result-PioneerGraphLeft(rounded,unsigned(shift)),halfway=PioneerGraphLeft(1,unsigned(shift-1));
		if (remainder>halfway || (remainder==halfway && (rounded&1))) rounded++;
	} else rounded=PioneerGraphLeft(result,unsigned(-shift));
	if (e< -126) return PioneerAttentionFloat(ps|uint32_t(rounded));
	if (rounded==0x1000000) { rounded>>=1; e++; }
	if (e>127) return PioneerAttentionFloat(ps|0x7f800000);
	return PioneerAttentionFloat(ps|(uint32_t(e+127)<<23)|(uint32_t(rounded)&0x7fffff));
}
#endif
