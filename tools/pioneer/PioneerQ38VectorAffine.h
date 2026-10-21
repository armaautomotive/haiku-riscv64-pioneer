/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_AFFINE_H
#define PIONEER_Q38_VECTOR_AFFINE_H
// OFFLINE arithmetic prerequisite ONLY;NO installed driver selects this helper.
// ALL32 lanes enabled. LReg0=canonical SM32 dot |dot|<=262144,1=coefficient,
// 2=minimum,3=activation scale,4=canonical SM32 input sum |sum|<=2048,
// 5=old zero-or-normal FP32 accumulator,|old|<=2^68. Integer zero MUST
// Separate Work-Q4 group32 qualification extends ONLY sum bound to4096;
// its paired integer dot is still within262144,all casts exact/normal.
// be positive zero. Metadata is
// exact finite half*[-128,127];scale is zero or |scale| in[2^-32,2^16].
// Future caller owes exact Dst/LReg loads,source-bank ownership,range/finite
// proofs and publication. This sequence alone authorizes NO native work.
// Preserve EVERY rounding point of the old RISC loop:
// old + ((coefficient*scale)*float(dot) + (minimum*scale)*float(sum)).
// Vendor8a34e25e:SFPMUL with negative-zero addend preserves signed zeros;
// SFPADD multiplies by exact1.0. Never fuse the affine chains. No denormal
// or overflow under the stated metadata/scale/accumulator domain.
template<class E> static inline void PioneerQ38VectorAffineMul(E& e,unsigned a,unsigned b,unsigned d)
{
	e.Op(0x86000902|(a<<16)|(b<<12)|(d<<4)); // C9 negated = -0 identity
	e.Op(0x8f000000); // explicit complete2-cycle MAD latency,also safe for later integer ops
}
template<class E> static inline void PioneerQ38VectorAffineAdd(E& e,unsigned a,unsigned b,unsigned d)
{
	e.Op(0x850a0000|(a<<12)|(b<<8)|(d<<4)); // A10 = exact1.0
	e.Op(0x8f000000);
}
template<class E> static void PioneerQ38VectorAffine(E& e)
{
	e.Op(0x90000000); e.Op(0x90000440); // RNE canonical SM32->FP32,exact for these bounded ints
	PioneerQ38VectorAffineMul(e,1,3,6);
	PioneerQ38VectorAffineMul(e,2,3,7);
	PioneerQ38VectorAffineMul(e,6,0,0);
	PioneerQ38VectorAffineMul(e,7,4,4);
	PioneerQ38VectorAffineAdd(e,0,4,0);
	PioneerQ38VectorAffineAdd(e,5,0,0);
}
// Separate OFFLINE variant:dot0 is already canonical exact FP32 integer
// from the lossless BF16 matrix path. SAME non-fused rounding points.
template<class E> static void PioneerQ38VectorAffineFloatDot(E& e)
{
	e.Op(0x90000440);
	PioneerQ38VectorAffineMul(e,1,3,6);
	PioneerQ38VectorAffineMul(e,2,3,7);
	PioneerQ38VectorAffineMul(e,6,0,0);
	PioneerQ38VectorAffineMul(e,7,4,4);
	PioneerQ38VectorAffineAdd(e,0,4,0);
	PioneerQ38VectorAffineAdd(e,5,0,0);
}
#endif
