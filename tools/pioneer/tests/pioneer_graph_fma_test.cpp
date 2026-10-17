/* SPDX-License-Identifier: MIT */
#include "../PioneerGraphFma.h"
#include "../PioneerGraphRope.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
static uint32_t state=0x792a6f31;
static uint32_t Next() { state^=state<<13; state^=state>>17; state^=state<<5; return state; }
static void Check(float a,float b,float c)
{
	uint32_t actual=PioneerAttentionBits(PioneerGraphFma(a,b,c)),expected=PioneerAttentionBits(fmaf(a,b,c));
	if (actual!=expected) { printf("FMA %08x %08x %08x actual%08x expected%08x\n",PioneerAttentionBits(a),PioneerAttentionBits(b),PioneerAttentionBits(c),actual,expected); fflush(stdout); }
	assert(actual==expected);
}
int main()
{
	for (unsigned i=0;i<2000000;i++) {
		float a=PioneerAttentionFloat(Next()),b=PioneerAttentionFloat(Next()),c=PioneerAttentionFloat(Next());
		if (isfinite(a) && isfinite(b) && isfinite(c)) Check(a,b,c);
	}
	for (unsigned i=0;i<300000;i++) {
		float a=PioneerAttentionFloat((Next()&0x807fffff)|((Next()%220+10)<<23));
		float b=PioneerAttentionFloat((Next()&0x807fffff)|((Next()%220+10)<<23));
		float c=-(a*b); if (isfinite(c)) { Check(a,b,c); Check(a,b,nextafterf(c,INFINITY)); Check(a,b,nextafterf(c,-INFINITY)); }
	}
	for (uint32_t a:{0U,0x80000000U,1U,0x80000001U,0x7fffffU,0x800000U,0x3f800000U,0xbf800000U,0x7f7fffffU})
		for (uint32_t b:{0U,0x80000000U,1U,0x3f000000U,0x3f800000U,0xbf800000U,0x7f7fffffU})
			for (uint32_t c:{0U,0x80000000U,1U,0x80000001U,0x7fffffU,0x800000U,0x3f800000U,0xbf800000U,0x7f7fffffU}) Check(PioneerAttentionFloat(a),PioneerAttentionFloat(b),PioneerAttentionFloat(c));
	unsigned fusedDifferences=0;
	for (unsigned trial=0;trial<512;trial++) {
		unsigned n=trial&1?2048:1024; std::vector<float> input(n),output(n); float table[128];
		for (unsigned i=0;i<n;i++) input[i]=float(int(Next()%200001)-100000)*.00013f;
		for (unsigned i=0;i<64;i++) { float theta=float(trial)*powf(1000000.0f,-float(i)/64); table[2*i]=cosf(theta); table[2*i+1]=sinf(theta); }
		assert(PioneerGraphRopeNeox(n,input.data(),table,output.data()));
		for (unsigned head=0;head<n;head+=128) for (unsigned i=0;i<64;i++) {
			float a=input[head+i],b=input[head+64+i],c=table[2*i],s=table[2*i+1];
			float x=fmaf(a,c,-(b*s)),y=fmaf(a,s,b*c);
			assert(PioneerAttentionBits(output[head+i])==PioneerAttentionBits(x) && PioneerAttentionBits(output[head+64+i])==PioneerAttentionBits(y));
			fusedDifferences+=PioneerAttentionBits(x)!=PioneerAttentionBits(a*c-b*s);
		}
		auto inplace=input; assert(PioneerGraphRopeNeox(n,inplace.data(),table,inplace.data()));
		for (unsigned i=0;i<n;i++) assert(PioneerAttentionBits(inplace[i])==PioneerAttentionBits(output[i]));
	}
	assert(fusedDifferences>10000);
	printf("PASS exact integer FMA:2M arbitrary finite cases,near cancellation,subnormal/overflow/signed zero,512 fused RoPE rows,in-place,unfused differences%u\n",fusedDifferences);
}
