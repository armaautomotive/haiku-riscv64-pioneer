/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <cmath>
#include <cstdio>
#include <stdint.h>

#include "../PioneerQwen35DataflowSilu.h"

using namespace PioneerQwen35;

static float Reference(float gate,float up)
{
	const double value=double(gate);
	const double sigmoid=value>=0
		?1.0/(1.0+std::exp(-value))
		:std::exp(value)/(1.0+std::exp(value));
	return float((value*sigmoid)*double(up));
}

int main()
{
	for (int step=0;step<=151000;step++) {
		const float input=-float(step)*0.001f;
		int actual=1;
		assert(DataflowSiluTruncateExponent(input,actual));
		assert(actual==int(std::trunc(input)));
	}
	int invalid=0;
	assert(!DataflowSiluTruncateExponent(0.5f,invalid));
	assert(!DataflowSiluTruncateExponent(-152.0f,invalid));
	assert(!DataflowSiluTruncateExponent(NAN,invalid));
	for (unsigned step=0;step<=104000;step++) {
		const float input=step==104000?0.0f:-104.0f+float(step)*0.001f;
		float got=0;
		if (!DataflowExpNonPositive(input,got)) {
			std::fprintf(stderr,"exp rejected input %.9g step %u\n",input,step);
			return 1;
		}
		const float expected=float(std::exp(double(input)));
		const float absolute=std::fabs(got-expected);
		const float relative=expected>1.0e-30f?absolute/expected:0;
		if (!(absolute<=2.0e-36f || relative<=2.0e-6f)) {
			std::fprintf(stderr,"exp mismatch x=%.9g got=%.9g expected=%.9g relative=%.9g\n",
				input,got,expected,relative);
			return 1;
		}
	}
	const float multipliers[]={-9.0f,-1.0f,-0.125f,0.0f,0.25f,1.0f,8.0f};
	for (int step=-100000;step<=100000;step++) {
		const float gate=float(step)*0.001f;
		for (float up:multipliers) {
			float got=0;
			assert(DataflowSiluProduct(gate,up,got));
			const float expected=Reference(gate,up);
			const float absolute=std::fabs(got-expected);
			const float tolerance=2.5e-6f*(1.0f+std::fabs(expected));
			if (!(absolute<=tolerance)) {
				std::fprintf(stderr,"silu mismatch gate=%.9g up=%.9g got=%.9g expected=%.9g\n",
					gate,up,got,expected);
				return 1;
			}
		}
	}
	float result=0;
	assert(!DataflowSiluProduct(NAN,1.0f,result));
	assert(!DataflowSiluProduct(1.0f,INFINITY,result));
	assert(!DataflowExpNonPositive(1.0f,result));
	return 0;
}
