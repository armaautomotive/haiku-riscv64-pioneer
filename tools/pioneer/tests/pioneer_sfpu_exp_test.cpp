/* SPDX-License-Identifier: MIT */
#define main PioneerSfpuDivideOriginalTests
#include "pioneer_sfpu_divide_test.cpp"
#undef main
#include "../PioneerSfpuExp.h"
#include <cmath>
#include <algorithm>
#include <vector>

int main()
{
	(void)PioneerSfpuDivideOriginalTests;
	NativeF32Q8Machine machine(1024,32,0);
	uint32_t seed=0x7f4a7c15;
	auto next=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
	unsigned common=0,fallback=0,maxUlp=0,halfDifferences=0;
	for(unsigned trial=0;trial<32768;trial++){
		float input[32],reference[32];bool useSfpu[32];
		for (unsigned row=0;row<12;row++) machine.valid[row]=true;
		for(unsigned lane=0;lane<32;lane++){
			float value=-float(next()%80000001)*0.000001f;
			if(trial<512)value=-float(trial%80)*0.999999f;
			if(trial>=512 && trial<1024)value=-float(lane)*0.001f;
			input[lane]=value;
			float reduced=0,factor=0;
			useSfpu[lane]=PioneerSfpuExpRange(value,reduced,factor);
			if(useSfpu[lane]){
				const unsigned row=lane/8,col=(lane&7)*2;
				machine.dst[row][col]=reduced;
				machine.dst[4+row][col]=factor;
				common++;
			}else{
				const unsigned row=lane/8,col=(lane&7)*2;
				machine.dst[row][col]=0.0f;
				machine.dst[4+row][col]=1.0f;
				fallback++;
			}
			reference[lane]=PioneerAttentionExp(value);
		}
		DivideEmitter emitter={machine};
		emitter.Op(0x70030000); // SFPLOAD reduced inputs from Dst rows 0..3
		emitter.Op(0x70130004); // SFPLOAD factors from Dst rows 4..7
		PioneerSfpuExp32(emitter);
		for(unsigned lane=0;lane<32;lane++){
			const unsigned row=8+lane/8,col=(lane&7)*2;
			float actual=useSfpu[lane]?machine.dst[row][col]:reference[lane];
			const unsigned a=Bits(actual),b=Bits(reference[lane]);
			const unsigned ulp=a>b?a-b:b-a;
			if(ulp>maxUlp)maxUlp=ulp;
			if(PioneerAttentionToHalf(actual)!=PioneerAttentionToHalf(reference[lane]))halfDifferences++;
		}
	}
	unsigned frames=0,probabilityHalfDifferences=0;double maxProbabilityError=0;
	for(unsigned length:{1U,31U,32U,33U,128U,512U,2048U,8192U})for(unsigned frame=0;frame<8;frame++){
		std::vector<float> logits(length),exact(length),candidate(length);
		float maximum=-INFINITY;
		for(unsigned i=0;i<length;i++){
			const float magnitude=float((i*7919U+frame*104729U+length*17U)%50000001U)*0.000001f;
			logits[i]=-magnitude;maximum=std::max(maximum,logits[i]);
		}
		double exactSum=0,candidateSum=0;
		for(unsigned first=0;first<length;first+=32){
			float reduced[32],factor[32],fallbackValue[32];bool use[32];
			for(unsigned lane=0;lane<32;lane++){
				const unsigned index=first+lane;
				const float value=index<length?logits[index]-maximum:0.0f;
				use[lane]=index<length?PioneerSfpuExpRange(value,reduced[lane],factor[lane]):false;
				if(index>=length){reduced[lane]=0.0f;factor[lane]=1.0f;}
				fallbackValue[lane]=use[lane]?0.0f:PioneerAttentionExp(value);
				const unsigned row=lane/8,col=(lane&7)*2;
				machine.dst[row][col]=reduced[lane];machine.dst[4+row][col]=factor[lane];
			}
			DivideEmitter emitter={machine};emitter.Op(0x70030000);emitter.Op(0x70130004);PioneerSfpuExp32(emitter);
			for(unsigned lane=0;lane<32 && first+lane<length;lane++){
				const float value=use[lane]?machine.dst[8+lane/8][(lane&7)*2]:fallbackValue[lane];
				exact[first+lane]=PioneerAttentionExp(logits[first+lane]-maximum);
				candidate[first+lane]=value;exactSum+=exact[first+lane];candidateSum+=value;
			}
		}
		for(unsigned i=0;i<length;i++){
			const double a=exact[i]/exactSum,b=candidate[i]/candidateSum;
			maxProbabilityError=std::max(maxProbabilityError,std::abs(a-b));
			if(PioneerAttentionToHalf(float(a))!=PioneerAttentionToHalf(float(b)))probabilityHalfDifferences++;
		}
		frames++;
	}
	// Deterministic stress patterns complement the seeded random frames. They
	// exercise equal-logit normalization, isolated/tied maxima, and the
	// polynomial/fallback boundary at very small and very large probabilities.
	for(unsigned length:{32U,33U,64U,1024U})for(unsigned pattern=0;pattern<6;pattern++){
		std::vector<float> logits(length),exact(length),candidate(length);
		for(unsigned i=0;i<length;i++){
			switch(pattern){
				case 0: logits[i]=0.0f; break; // all equal
				case 1: logits[i]=i==0?0.0f:-80.0f; break; // one dominant token
				case 2: logits[i]=(i%2)==0?0.0f:-80.0f; break; // tied peaks
				case 3: logits[i]=-float(i%81); break; // integer-spaced tail
				case 4: logits[i]=-float(i%257)*0.000001f; break; // near-equal
				default: logits[i]=i%13==0?0.0f:-79.999f; break;
			}
		}
		const float maximum=*std::max_element(logits.begin(),logits.end());
		double exactSum=0,candidateSum=0;
		for(unsigned first=0;first<length;first+=32){
			float reduced[32],factor[32],fallbackValue[32];bool use[32];
			for(unsigned lane=0;lane<32;lane++){
				const unsigned index=first+lane;
				const float value=index<length?logits[index]-maximum:0.0f;
				use[lane]=index<length?PioneerSfpuExpRange(value,reduced[lane],factor[lane]):false;
				if(index>=length){reduced[lane]=0.0f;factor[lane]=1.0f;}
				fallbackValue[lane]=use[lane]?0.0f:PioneerAttentionExp(value);
				const unsigned row=lane/8,col=(lane&7)*2;
				machine.dst[row][col]=reduced[lane];machine.dst[4+row][col]=factor[lane];
			}
			DivideEmitter emitter={machine};emitter.Op(0x70030000);emitter.Op(0x70130004);PioneerSfpuExp32(emitter);
			for(unsigned lane=0;lane<32 && first+lane<length;lane++){
				const float value=use[lane]?machine.dst[8+lane/8][(lane&7)*2]:fallbackValue[lane];
				exact[first+lane]=PioneerAttentionExp(logits[first+lane]-maximum);
				candidate[first+lane]=value;exactSum+=exact[first+lane];candidateSum+=value;
			}
		}
		for(unsigned i=0;i<length;i++){
			const double a=exact[i]/exactSum,b=candidate[i]/candidateSum;
			maxProbabilityError=std::max(maxProbabilityError,std::abs(a-b));
			if(PioneerAttentionToHalf(float(a))!=PioneerAttentionToHalf(float(b)))probabilityHalfDifferences++;
		}
		frames++;
	}
	printf("PASS OFFLINE Dst-load/SFPU-exp/Dst-store:lanes%u/fallback%u/maxULP%u/expHalfDifferences%u; softmaxFrames%u/maxProbabilityAbsError%.9g/probabilityHalfDifferences%u; degree8 vs exact degree10, NOT Qwen quality or hardware speed\n",
		common,fallback,maxUlp,halfDifferences,frames,maxProbabilityError,probabilityHalfDifferences);
}
