/* SPDX-License-Identifier: Apache-2.0 */
// Based on Tenstorrent's Apache-2.0 bit-perfect Blackhole FMA model:
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/Miscellaneous/FMA/fma.c
// SPDX-FileCopyrightText: Copyright 2025 Tenstorrent AI ULC
// License text: ../../data/system/data/licenses/Apache v2
// Modified: finite-Q8 specialization and a new CPU-only precision candidate.
#ifndef PIONEER_BLACKHOLE_FMA_H
#define PIONEER_BLACKHOLE_FMA_H
#include <stdint.h>
#include <string.h>
#include <assert.h>

static inline uint32_t PioneerFmaBits(float x) { uint32_t b; memcpy(&b,&x,4); return b; }
static inline float PioneerFmaFloat(uint32_t b) { float x; memcpy(&x,&b,4); return x; }
// Diagnostic only. Q8 finite scales/dots never overflow FP32. Denormal input
// and output are flushed just as in SFPMAD. This is NOT a device operation.
static inline uint32_t PioneerBlackholeFmaBits(uint32_t x,uint32_t y,uint32_t z) {
	int xe=int((x>>23)&255),ye=int((y>>23)&255),ze=int((z>>23)&255);
	assert(xe!=255 && ye!=255 && ze!=255);
	uint64_t xm=xe?((x&0x7fffff)|0x800000):0,ym=ye?((y&0x7fffff)|0x800000):0;
	uint32_t zm=ze?(((z&0x7fffff)|0x800000)<<3):0;
	uint64_t full=(xm*ym)<<3;
	uint32_t pm=uint32_t(full>>23)|((full&0x7fffff)!=0);
	int pe=xe+ye-127;
	uint32_t ps=(x^y)&0x80000000,zs=z&0x80000000;
	assert(pe<255);
	if (!pm || pe<0) return zm?z:(ps&zs);
	int re=pe>ze?pe:ze;
	unsigned shift=unsigned(re-pe);
	if (shift) { uint32_t old=pm; pm=shift>=32?0:pm>>shift; if (pm) pm|=(uint64_t(pm)<<shift)!=old; }
	shift=unsigned(re-ze);
	if (shift) { uint32_t old=zm; zm=shift>=32?0:zm>>shift; if (zm) zm|=(uint64_t(zm)<<shift)!=old; }
	uint32_t sign=pm>=zm?ps:zs;
	if (ps!=sign) pm=~pm;
	if (zs!=sign) zm=~zm;
	uint32_t rm=pm+zm+(ps!=zs);
	if (!rm) return ps&zs;
	int n=5-__builtin_clz(rm); re+=n;
	assert(re<255);
	if (re<=0) { n++; re=0; }
	if (n<=0) rm<<=-n; else rm=(rm>>n)|((rm&unsigned(n|1))!=0);
	uint32_t result=(uint32_t(re)<<23)+((rm>>3)&0x7fffff);
	result+=((rm&7)+(result&1))>4;
	if (!(result>>23)) result=0;
	return sign|result;
}
static inline float PioneerBlackholeFma(float a,float b,float c) {
	return PioneerFmaFloat(PioneerBlackholeFmaBits(PioneerFmaBits(a),PioneerFmaBits(b),PioneerFmaBits(c)));
}
// CPU-only candidate for a future SFPU sequence. Each primitive below is
// the partial hardware model, NOT host FMA. No real backend uses this yet.
static inline float PioneerBlackholeAccurateFma(float a,float b,float c) {
	float ah=PioneerFmaFloat(PioneerFmaBits(a)&~0xfffU),bh=PioneerFmaFloat(PioneerFmaBits(b)&~0xfffU);
	float al=PioneerBlackholeFma(a,1,-ah),bl=PioneerBlackholeFma(b,1,-bh);
	float p=PioneerBlackholeFma(a,b,0);
	float e=PioneerBlackholeFma(ah,bh,-p);
	e=PioneerBlackholeFma(al,bh,e);
	e=PioneerBlackholeFma(ah,bl,e);
	e=PioneerBlackholeFma(al,bl,e);
	float s=PioneerBlackholeFma(p,1,c),v=PioneerBlackholeFma(s,1,-p);
	float t=PioneerBlackholeFma(PioneerBlackholeFma(p,1,-PioneerBlackholeFma(s,1,-v)),1,PioneerBlackholeFma(c,1,-v));
	float u=PioneerBlackholeFma(e,1,t),w=PioneerBlackholeFma(u,1,-e);
	float low=PioneerBlackholeFma(PioneerBlackholeFma(e,1,-PioneerBlackholeFma(u,1,-w)),1,PioneerBlackholeFma(t,1,-w));
	uint32_t ub=PioneerFmaBits(u),lb=PioneerFmaBits(low);
	if ((lb&0x7fffffff) && !(ub&1)) ub+=((ub^lb)&0x80000000)?uint32_t(-1):1;
	return PioneerBlackholeFma(s,1,PioneerFmaFloat(ub));
}
#endif
