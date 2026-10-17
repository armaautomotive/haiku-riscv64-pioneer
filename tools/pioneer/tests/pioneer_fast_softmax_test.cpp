/* SPDX-License-Identifier: MIT */
#include "../PioneerFastSoftmax.h"
#include <cmath>
#include <cstdio>
#include <cassert>
#include <initializer_list>
static uint32_t seed=0x36f18ac3;
static uint32_t Random() { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; }
int main() {
	unsigned cases=0,different=0; double maximum=0;
	auto check=[&](float input) {
		float actual=PioneerFastSoftmaxExp(input),reference=input<=-80?0:float(std::exp(double(input)));
		assert(std::isfinite(actual) && actual>=0 && actual<=1);
		if (reference) {
			double relative=fabs(double(actual)/reference-1); if (relative>maximum) maximum=relative;
			assert(relative<3e-7);
		} else assert(PioneerAttentionBits(actual)==0);
		different+=PioneerAttentionBits(actual)!=PioneerAttentionBits(reference); cases++;
	};
	for (unsigned i=0;i<1048576;i++) check(-80.0f*float(Random()&0xffffff)/16777216.0f);
	for (unsigned n=0;n<116;n++) {
		float center=-float((double(n)+.5)*log(2.0));
		for (int delta=-64;delta<=64;delta++) {
			float input=PioneerAttentionFloat(PioneerAttentionBits(center)+uint32_t(delta)); if (input<=0) check(input);
		}
	}
	for (float x:{0.0f,-0.0f,-80.0f,-81.0f,-3.402823466e38f,-1e-35f}) check(x);
	for (float x:{1e-30f,1.0f,INFINITY,-INFINITY,NAN}) assert(std::isnan(PioneerFastSoftmaxExp(x)));
	printf("PASS OFFLINE stable-softmax single-float polynomial,%u cases,max relative%.9g,%u non-bit-identical reference results;NOT full-model/native qualification\n",cases,maximum,different);
}
