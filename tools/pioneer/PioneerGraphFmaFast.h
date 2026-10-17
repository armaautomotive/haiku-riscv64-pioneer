/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GRAPH_FMA_FAST_H
#define PIONEER_GRAPH_FMA_FAST_H
#include "PioneerGraphFma.h"
// OFFLINE arithmetic prerequisite,NOT selected by any native image yet.
// Same split-product/two-sum/sticky-odd idea as the qualified SFPU exact
// sequence,but scalar operations only. Explicit bounded domain ensures
// EVERY nonzero intermediate is normal FP32 on a flushing BRISC.
static inline bool PioneerGraphFmaCompensated(float a,float b,float c,float& output)
{
	uint32_t aa=PioneerAttentionBits(a)&0x7fffffff,bb=PioneerAttentionBits(b)&0x7fffffff,cc=PioneerAttentionBits(c)&0x7fffffff;
	if (aa<0x35800000 || aa>0x477fe000 || bb<0x30800000 || bb>0x3f800000
		|| cc>0x477fe000 || (cc && cc<0x26800000)) return false;
	float ah=PioneerAttentionFloat(PioneerAttentionBits(a)&~0xfffU),bh=PioneerAttentionFloat(PioneerAttentionBits(b)&~0xfffU);
	float al=a-ah,bl=b-bh,p=a*b,s=p+c;
	if ((PioneerAttentionBits(s)&0x7fffffff)<0x26800000) return false;
	float e=ah*bh-p; e=e+al*bh; e=e+ah*bl; e=e+al*bl;
	float v=s-p,t=(p-(s-v))+(c-v),u=e+t,w=u-e;
	float low=(e-(u-w))+(t-w);
	uint32_t ub=PioneerAttentionBits(u),lb=PioneerAttentionBits(low);
	if ((lb&0x7fffffff) && !(ub&1)) ub+=((ub^lb)&0x80000000)?uint32_t(-1):1;
	output=s+PioneerAttentionFloat(ub); return true;
}
static inline float PioneerGraphFmaHybrid(float a,float b,float c)
{
	float output=0;
	return PioneerGraphFmaCompensated(a,b,c,output)?output:PioneerGraphFma(a,b,c);
}
#endif
