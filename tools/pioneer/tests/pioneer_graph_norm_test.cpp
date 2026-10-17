/* SPDX-License-Identifier: MIT */
#include "../PioneerGraphNorm.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
static uint32_t state=0x89abcdef;
static uint32_t Next() { state^=state<<13; state^=state>>17; state^=state<<5; return state; }
int main()
{
	{ PioneerAttentionWide full={0,0},fast={0,0};
		for (unsigned i=0;i<2000000;i++) {
			if (!(i&1023)) full=fast={0,0};
			float term=i%31?PioneerAttentionFloat(((Next()%132+27)<<23)|(Next()&0x7fffff)):0;
			full=PioneerAttentionWideAdd(full,{term,0}); fast=PioneerGraphRmsAddPositive(fast,term);
			assert(PioneerAttentionBits(full.high)==PioneerAttentionBits(fast.high) && full.low==fast.low);
		}
	}
	for (unsigned i=0;i<2000000;i++) {
		uint32_t bits=Next()&0x7fffffff; if (bits>=0x7f800000) continue;
		float x=PioneerAttentionFloat(bits); assert(PioneerAttentionBits(PioneerGraphSqrtPositive(x))==PioneerAttentionBits(sqrtf(x)));
	}
	for (unsigned exponent=0;exponent<255;exponent++) for (unsigned tail:{0U,1U,0x3fffffU,0x7ffffeU,0x7fffffU}) {
		float x=PioneerAttentionFloat((exponent<<23)|tail); assert(PioneerAttentionBits(PioneerGraphSqrtPositive(x))==PioneerAttentionBits(sqrtf(x)));
	}
	assert(PioneerAttentionBits(PioneerGraphSqrtPositive(-0.0f))==0x80000000);
	assert(PioneerAttentionBits(PioneerGraphSqrtPositive(-1.0f))==0x7fc00000);
	assert(PioneerAttentionBits(PioneerGraphSqrtPositive(INFINITY))==0x7f800000);
	unsigned differences=0,maxUlp=0;
	for (unsigned n:{128U,1024U}) for (unsigned trial=0;trial<20000;trial++) {
		std::vector<float> input(n),output(n); double sum=0;
		for (auto& value:input) {
			unsigned kind=trial%8;
			value=kind==0?float(int(Next()%200001)-100000)*.00013f:kind==1?PioneerAttentionFloat((Next()&0x807fffff)|((unsigned(Next()%50)+80)<<23))
				:kind==2?1.0f:kind==3?-0.0f:kind==4?65504.0f:kind==5?PioneerAttentionFloat(0x26800000):float(int(Next()%200001)-100000)*.13f;
			sum+=double(value*value);
		}
		float mean=float(sum/n),epsilon=trial%3?1e-6f:1e-5f,scale=1.0f/sqrtf(mean+epsilon);
		assert(PioneerGraphRmsNorm(n,input.data(),epsilon,output.data()));
		for (unsigned i=0;i<n;i++) { uint32_t a=PioneerAttentionBits(output[i]),b=PioneerAttentionBits(input[i]*scale);
			unsigned ulp=a>b?a-b:b-a; if (ulp) differences++; if (ulp>maxUlp) maxUlp=ulp; assert(!ulp); }
	}
	float input[128]={},output[128]={};
	assert(!PioneerGraphRmsNorm(128,input,0,output)); input[0]=INFINITY; assert(!PioneerGraphRmsNorm(128,input,1e-6f,output));
	input[0]=PioneerAttentionFloat(1); assert(!PioneerGraphRmsNorm(128,input,1e-6f,output));
	assert(!PioneerGraphRmsNorm(127,input,1e-6f,output) && !PioneerGraphRmsNorm(128,input,-1e-6f,output));
	printf("PASS OFFLINE graph norm:2M exact sqrt,edge exponents,40000 bounded pinned-CPU RMS rows,FP32 differences%u maxULP%u;NO device/decoder exposure\n",differences,maxUlp);
}
