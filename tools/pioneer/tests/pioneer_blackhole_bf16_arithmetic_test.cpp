/* SPDX-License-Identifier: MIT */
#include "../PioneerBlackholeBf16Arithmetic.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
	uint16_t a[16]={},b[16]={};uint32_t result=0x12345678;
	unsigned phase,previous,expected,cases=0;
	while(scanf("%u %x %x",&phase,&previous,&expected)==3) {
		for(auto& v:a){unsigned x;assert(scanf("%x",&x)==1 && x<=65535);v=uint16_t(x);}
		for(auto& v:b){unsigned x;assert(scanf("%x",&x)==1 && x<=65535);v=uint16_t(x);}
		assert(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,previous,result));
		if(result!=expected)fprintf(stderr,"ARITHMETIC_CASE_MISMATCH case%u phase%u previous%08x got%08x expected%08x\n",cases,phase,previous,result,expected);
		assert(result==expected);cases++;
	}
	assert(feof(stdin) && cases>=20000);unsigned refused=0;
	for(unsigned operand=0;operand<2;operand++)for(unsigned index=0;index<16;index++)for(unsigned bad:{1U,0x7f80U,0xff81U}) {
		for(auto& v:a)v=0x3f80;for(auto& v:b)v=0x3f80;
		(operand?b:a)[index]=uint16_t(bad);result=0x12345678;
		assert(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,0,0,result) && result==0x12345678);refused++;
	}
	for(auto& v:a)v=0x3f80;for(auto& v:b)v=0x3f80;
	for(unsigned bad:{1U,0x80000001U,0x7fc00000U,0xff800001U}) {
		result=0x12345678;assert(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,0,bad,result) && result==0x12345678);refused++;
	}
	for(unsigned bad:{4U,~0U}){result=0x12345678;assert(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,bad,0,result) && result==0x12345678);refused++;}
	printf("PASS separate Blackhole BF16/FP32 integer characterization:%u independent Fraction cases,%u atomic refusals;NO_DEVICE_OR_HARDWARE_ACCEPTANCE_AUTHORITY\n",cases,refused);
}
