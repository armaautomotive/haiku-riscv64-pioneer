/* SPDX-License-Identifier: MIT */
// OFFLINE arithmetic investigation ONLY. No image,driver or model selector.
// Q4_K shares coefficient/minimum across two adjacent16-input panels.
// Combining their EXACT integer dots BEFORE FP32 affine halves the number
// of affine updates,but intentionally changes FP32 rounding. NEVER label
// this bit-exact to the existing16-input implementation or GGML inference.
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "../PioneerQwen38Panel.h"
static float Float(uint32_t u) { float f; std::memcpy(&f,&u,4); return f; }
static uint32_t Bits(float f) { uint32_t u; std::memcpy(&u,&f,4); return u; }
static int Operand(uint16_t v) { return int(v&1023)*((v&0x8000)?-1:1); }
static uint32_t Random(uint32_t& s) { s^=s<<13;s^=s>>17;s^=s<<5;return s; }
static float Affine(float old,float coefficient,float minimum,float scale,int dot,int sum) {
	volatile float c=coefficient*scale,m=minimum*scale;
	volatile float p=c*float(dot),q=m*float(sum),v=p+q;
	return old+v;
}
int main() {
	uint32_t seed=0x38473201; unsigned different=0,outputs=0;
	double worstOld=0,worstPair=0,worstChange=0;
	const uint16_t halves[]={0,0x8000,1,0x8001,0x3ff,0x400,0x3c00,0xbc00,0x7bff,0xfbff};
	for (unsigned trial=0;trial<192;trial++) for(unsigned inputs:{5120U,6144U,17408U}) {
		std::array<float,16> old{},paired{};std::array<double,16> reference{},expanded{};
		for(unsigned block=0;block<inputs/256;block++) {
			std::array<uint8_t,2304> tile;std::array<int,256> input;
			for(auto& b:tile)b=uint8_t(Random(seed));
			for(unsigned row=0;row<16;row++)for(unsigned kind=0;kind<2;kind++) {
				uint16_t h=halves[(trial+block*7+row*3+kind)%10];
				tile[kind*32+row]=uint8_t(h);tile[kind*32+row+16]=uint8_t(h>>8);
			}
			for(unsigned k=0;k<256;k++) input[k]=trial%8==0?0:trial%8==1?-128:trial%8==2?127:int(Random(seed)%256)-128;
			float scale=Float(((95+Random(seed)%49)<<23)|(Random(seed)&0x807fffff));
			for(unsigned group=0;group<8;group++) {
				PioneerQ38Panel panel[2];int dot[2][16]={},sum[2]={};
				for(unsigned p=0;p<2;p++) {
					assert(PioneerQ38UnpackPanel(12,tile.data(),2304,group*2+p,&panel[p]));
					for(unsigned k=0;k<16;k++) {
						int x=input[group*32+p*16+k];sum[p]+=x;
						for(unsigned row=0;row<16;row++) {
							unsigned e=k*16+row;int w=Operand(uint16_t(panel[p].operand[e/2]>>((e%2)*16)));
							dot[p][row]+=w*x;
							double c=double(Float(panel[p].coefficient[row]))*scale*w*x;
							double m=double(Float(panel[p].minimum[row]))*scale*x;
							reference[row]+=c+m;expanded[row]+=std::fabs(c)+std::fabs(m);
						}
					}
					assert(sum[p]>=-2048 && sum[p]<=2032);
					for(unsigned row=0;row<16;row++) {
						assert(dot[p][row]>=-30720 && dot[p][row]<=30480);
						old[row]=Affine(old[row],Float(panel[p].coefficient[row]),Float(panel[p].minimum[row]),scale,dot[p][row],sum[p]);
					}
				}
				assert(sum[0]+sum[1]>=-4096 && sum[0]+sum[1]<=4064);
				for(unsigned row=0;row<16;row++) {
					assert(panel[0].coefficient[row]==panel[1].coefficient[row] && panel[0].minimum[row]==panel[1].minimum[row]);
					paired[row]=Affine(paired[row],Float(panel[0].coefficient[row]),Float(panel[0].minimum[row]),scale,dot[0][row]+dot[1][row],sum[0]+sum[1]);
				}
			}
		}
		for(unsigned row=0;row<16;row++) {
			assert(std::isfinite(old[row]) && std::isfinite(paired[row]));
			double e0=std::fabs(double(old[row])-reference[row]),e1=std::fabs(double(paired[row])-reference[row]);
			double nu=double(inputs/16+8)*0x1p-24,bound=1e-6+nu/(1-nu)*expanded[row];
			assert(e0<=bound && e1<=bound);outputs++;different+=Bits(old[row])!=Bits(paired[row]);
			if(expanded[row]) {worstOld=std::fmax(worstOld,e0/expanded[row]);worstPair=std::fmax(worstPair,e1/expanded[row]);worstChange=std::fmax(worstChange,std::fabs(double(old[row])-paired[row])/expanded[row]);}
		}
	}
	assert(different && different<outputs);
	printf("PASS OFFLINE group32 arithmetic outputs%u changed_rounding%u worst_error_over_expanded old%.9g pair%.9g change%.9g;INDEPENDENT_FP64_EXPANDED_BOUND_NOT_RELATIVE_OUTPUT_ERROR;NO_IMAGE_NATIVE_MODEL_QUALITY_OR_SPEED_QUALIFICATION\n",outputs,different,worstOld,worstPair,worstChange);
}
