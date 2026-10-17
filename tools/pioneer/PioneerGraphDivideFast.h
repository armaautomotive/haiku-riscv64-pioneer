/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GRAPH_DIVIDE_FAST_H
#define PIONEER_GRAPH_DIVIDE_FAST_H
#include "PioneerGraphFma.h"
// OFFLINE prerequisite. Approximation proposes a significand ONLY. Exact
// integer product/remainder correction and one RNE determine the result.
// Every floating operation stays in [roughly .25,4],never denormal. Zero,
// subnormal inputs/results and exceptional cases use the old exact path.
static inline bool PioneerGraphDivideNormalFast(float a,float b,float& output)
{
	uint32_t aa=PioneerAttentionBits(a),bb=PioneerAttentionBits(b);
	if (aa<0x00800000 || aa>=0x7f800000 || bb<0x00800000 || bb>=0x7f800000) return false;
	uint32_t ma=(aa&0x7fffff)|0x800000,mb=(bb&0x7fffff)|0x800000;
	int exponent=int(aa>>23)-int(bb>>23)+127-(ma<mb);
	if (exponent<1 || exponent>254) return false;
	float an=PioneerAttentionFloat((aa&0x7fffff)|0x3f800000),bn=PioneerAttentionFloat((bb&0x7fffff)|0x3f800000);
	float inverse=PioneerAttentionFloat(0x7ef311c3U-PioneerAttentionBits(bn));
	for (unsigned i=0;i<4;i++) inverse=inverse*(2.0f-bn*inverse);
	float estimate=an*inverse; if (ma<mb) estimate=estimate*2.0f;
	uint32_t bits=PioneerAttentionBits(estimate),m=(bits&0x7fffff)|0x800000;
	unsigned e=bits>>23;
	if (e==126) m>>=1; else if (e==128) m<<=1; else if (e!=127) return false;
	uint32_t p0=(m&4095)*(mb&4095),p1=(m>>12)*(mb&4095)+(m&4095)*(mb>>12)+(p0>>12);
	uint32_t p2=(m>>12)*(mb>>12)+(p1>>12);
	PioneerGraphU64 product=(PioneerGraphU64(p2)<<24)|((p1&4095)<<12)|(p0&4095),numerator=PioneerGraphU64(ma)<<23;
	if (ma<mb) numerator+=numerator;
	bool bounded=false;
	for (unsigned step=0;step<8;step++) {
		if (product>numerator) { product-=mb; m--; }
		else if (numerator-product>=mb) { product+=mb; m++; }
		else { bounded=true; break; }
	}
	if (!bounded) return false; // Bounded arithmetic fallback,NOT a device retry.
	uint32_t remainder=uint32_t(numerator-product);
	if (remainder*2>mb || (remainder*2==mb && (m&1))) m++;
	if (m==0x1000000) { m>>=1; exponent++; }
	output=PioneerAttentionFloat((uint32_t(exponent)<<23)|(m&0x7fffff)); return true;
}
#endif
