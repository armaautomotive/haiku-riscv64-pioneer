/* SPDX-License-Identifier: MIT */
#include "../PioneerGraphMath.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static uint32_t randomState=0x12345678;
static uint32_t Next() { randomState^=randomState<<13; randomState^=randomState>>17; randomState^=randomState<<5; return randomState; }
int main()
{
	for (unsigned i=0;i<1000000;i++) {
		uint32_t a=Next()&0x7fffffff,b=Next()&0x7fffffff;
		if (a>=0x7f800000 || b>=0x7f800000 || !b) continue;
		float x=PioneerAttentionFloat(a),y=PioneerAttentionFloat(b);
		float expected=x/y,actual=PioneerGraphDividePositive(x,y);
		assert(PioneerAttentionBits(expected)==PioneerAttentionBits(actual));
	}
	unsigned maxUlp=0,maxFastUlp=0;
	for (int i=-32000;i<=32000;i++) {
		float x=float(i)/1000.0f,expected=x/(1.0f+expf(-x)),actual=PioneerGraphSilu(x);
		assert(isfinite(actual) && fabsf(actual-expected)<2e-6f*(1+fabsf(expected)));
		uint32_t a=PioneerAttentionBits(actual),b=PioneerAttentionBits(expected); unsigned ulp=a>b?a-b:b-a; if (ulp>maxUlp) maxUlp=ulp;
		float fast=PioneerGraphSiluFast(x); uint32_t f=PioneerAttentionBits(fast); unsigned fastUlp=f>b?f-b:b-f; if (fastUlp>maxFastUlp) maxFastUlp=fastUlp;
		assert(isfinite(fast) && fabsf(fast-expected)<2e-6f*(1+fabsf(expected)));
	}
	for (unsigned trial=0;trial<20000;trial++) {
		float values[32]; float maximum=0;
		for (auto& value:values) { value=float(int(Next()%200001)-100000)*.00013f; maximum=fmaxf(maximum,fabsf(value)); }
		if (!trial) for (auto& value:values) value=0;
		if (!trial) maximum=0;
		uint32_t operand[16],scale=0; assert(PioneerGraphQuantize32(values,operand,scale));
		float d=maximum/127.0f,id=d?1.0f/d:0;
		assert(scale==PioneerAttentionBits(PioneerAttentionHalf(PioneerAttentionToHalf(d))));
		for (unsigned i=0;i<32;i++) assert(uint16_t(operand[i/2]>>(16*(i&1)))==PioneerInt8Operand(int(roundf(values[i]*id))));
	}
	assert(PioneerAttentionBits(PioneerGraphSilu(-0.0f))==0x80000000);
	assert(PioneerAttentionBits(PioneerGraphSilu(PioneerAttentionFloat(0x7f800000)))==0x7fc00000);
	printf("PASS OFFLINE graph math: one million IEEE divisions,SiLU debug/fast max%u/%u ULP,20000 exact pinned Q8-ref blocks,zero/sign/nonfinite bounds; NO driver/device/inference exposure\n",maxUlp,maxFastUlp);
	return 0;
}
