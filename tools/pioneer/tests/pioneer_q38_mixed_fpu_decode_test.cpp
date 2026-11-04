/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38MixedFpuDecode.h"
#include "../PioneerBlackholeFma.h"
#include "../PioneerQ38BinaryScale.h"
#include "../TenstorrentGgufQuant.h"
#include <array>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cfenv>
struct MixedFpuWords {std::vector<uint32_t> words;void Op(uint32_t w){words.push_back(w);}};
// Separate strict register/latency observer. Does not broaden the qualified
// Q4 observer,RV32 decoder,UNPACK model or any physical admission predicate.
struct MixedFpuMachine {
	uint32_t lanes[8][32]={};unsigned ready[8]={},cycle=0;bool okay=true;
	uint32_t Read(unsigned r,unsigned lane) {
		if(r==9)return 0;if(r==10)return 0x3f800000;
		assert(r<8);if(ready[r]>cycle)okay=false;return lanes[r][lane];
	}
	void Op(uint32_t w) {
		if(!okay)return;if(w==0x8f000000){cycle++;return;}
		unsigned code=w>>24,d=(w>>4)&15,c=(w>>8)&15,b=(w>>12)&15,a=(w>>16)&15,mod=w&15;
		if(code==0x71)d=(w>>20)&15;
		assert(d<8);uint32_t out[32]={};unsigned latency=1;
		for(unsigned lane=0;lane<32;lane++) {
			if(code==0x71) {
				unsigned mode=(w>>16)&15;assert(mode==0 || mode==2 || mode==10);
				out[lane]=mode==0?(w&65535)<<16:mode==2?w&65535:(Read(d,lane)&0xffff0000)|(w&65535);
			} else if(code==0x90) {
				assert(!(w&0x00fff00f) && c<8);uint32_t sm=Read(c,lane);assert(sm<=127);
				out[lane]=PioneerFmaBits(float(sm));
			} else if(code==0x86 || code==0x85 || code==0x84) {
				assert((code==0x86 && c==9 && mod==2) || (code==0x85 && a==10 && !mod)
					|| (code==0x84 && w==0x84043000));
				uint32_t x=Read(a,lane),y=Read(b,lane),z=Read(c,lane);if(mod&2)z^=0x80000000;
				out[lane]=PioneerBlackholeFmaBits(x,y,z);latency=2;
			} else if(code==0x7c){assert(!mod && c<8);out[lane]=Read(c,lane);}
			else if(code==0x7a) {
				assert(mod==1 && c==d);int amount=int(w<<8)>>20;
				assert(amount==-2 || amount==-4 || amount==-6 || amount==-7 || amount==-16 || amount==4);
				out[lane]=amount<0?Read(d,lane)>>unsigned(-amount):Read(d,lane)<<unsigned(amount);
			} else if(code==0x7e){assert(mod==1 && b<8 && c<8);out[lane]=Read(b,lane)&Read(c,lane);}
			else if(code==0x79){assert(mod==4 && c<8);out[lane]=Read(d,lane)+Read(c,lane);}
			else {fprintf(stderr,"UNKNOWN_MIXED_FPU_OPCODE %08x\n",w);assert(false);}
		}
		for(unsigned lane=0;lane<32;lane++)lanes[d][lane]=out[lane];
		ready[d]=cycle+latency;cycle++;
	}
	void Execute(const MixedFpuWords& words){for(uint32_t w:words.words)Op(w);}
};
static uint32_t MixedRound(uint32_t f) {
	assert((f&0x7f800000)!=0x7f800000 && (!(f&0x7fffffff) || (f&0x7f800000)));
	uint32_t high=f>>16,low=f&65535;
	if(low>32768 || (low==32768 && (high&1)))high++;
	return high<<16;
}
struct MixedFpuPrograms {
	MixedFpuWords q6Prepare,q8Prepare,q6[4],q8;
	MixedFpuPrograms() {
		PioneerQ38Q6FpuPrepare(q6Prepare);PioneerQ38Q8FpuPrepare(q8Prepare);
		PioneerQ38Q6FpuDecode<0>(q6[0]);PioneerQ38Q6FpuDecode<1>(q6[1]);
		PioneerQ38Q6FpuDecode<2>(q6[2]);PioneerQ38Q6FpuDecode<3>(q6[3]);PioneerQ38Q8FpuDecode(q8);
	}
};
#ifndef PIONEER_MIXED_FPU_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(std::fegetround()==FE_TONEAREST && (argc==1 || (argc==2 && !strcmp(argv[1],"--fraction-fixture"))));
	const bool fraction=argc==2;MixedFpuPrograms p;uint64_t q6Cases=0,q8Cases=0;
	assert(p.q6Prepare.words.size()==6 && p.q8Prepare.words.size()==7);
	assert(p.q6[0].words.size()==17 && p.q6[1].words.size()==18 && p.q6[2].words.size()==19 && p.q6[3].words.size()==19 && p.q8.words.size()==15);
	uint32_t seed=0x6b893f27;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
	const unsigned trials=fraction?32:2048;
	std::array<bool,65536> halfSeen{};std::array<bool,256> scaleSeen{},q8Seen{};std::array<bool,64> q6Seen{};
	unsigned negativeZero=0,minimumQ8=0;
	for(unsigned trial=0;trial<trials;trial++) {
		MixedFpuMachine q6,q8;uint16_t half[32];int scale[32];
		for(unsigned lane=0;lane<32;lane++) {
			unsigned finite=(trial*32+lane)%63488;
			half[lane]=uint16_t((finite%31744)|((finite/31744)<<15));
			if(fraction)half[lane]=uint16_t((random()%0x7c00)|(random()&0x8000));
			if(fraction && trial<2)half[lane]=uint16_t(trial?0x8000:0);
			if(fraction && (trial==2 || trial==3))half[lane]=uint16_t((trial==3?0x8000:0)+lane+1);
			scale[lane]=int((trial*19+lane*37)&255)-128;
			float coefficient=PioneerHalfFloat(half[lane])*float(scale[lane]);uint32_t prepared;
			assert(PioneerQ38BinaryScale(half[lane],scale[lane],prepared) && prepared==PioneerFmaBits(coefficient));
			q6.lanes[1][lane]=prepared;q8.lanes[1][lane]=PioneerFmaBits(PioneerHalfFloat(half[lane]));
			halfSeen[half[lane]]=true;scaleSeen[unsigned(scale[lane]+128)]=true;
		}
		q6.Execute(p.q6Prepare);q8.Execute(p.q8Prepare);
		for(unsigned batch=0;batch<8;batch++) {
			for(unsigned group=0;group<4;group++) {
				unsigned lo[32],hi[32];uint32_t keep[8][32];
				for(unsigned lane=0;lane<32;lane++) {
					lo[lane]=batch*32+lane;hi[lane]=random()&255;
					if(!batch)hi[lane]=0;if(batch==7)hi[lane]=255;
					q6.lanes[0][lane]=lo[lane];q6.lanes[3][lane]=hi[lane];
					for(unsigned reg:{1U,2U,4U,5U,6U,7U})keep[reg][lane]=q6.lanes[reg][lane];
				}
				q6.Execute(p.q6[group]);assert(q6.okay);
				for(unsigned lane=0;lane<32;lane++) {
					int quant=int(((lo[lane]>>((group/2)*4))&15)|(((hi[lane]>>(group*2))&3)<<4))-32;
					float coefficient=PioneerHalfFloat(half[lane])*float(scale[lane]);
					uint32_t expected=MixedRound(PioneerFmaBits(coefficient*float(quant)));
					assert(q6.lanes[0][lane]==expected);
					for(unsigned reg:{1U,2U,4U,5U,6U,7U})assert(keep[reg][lane]==q6.lanes[reg][lane]);
					q6Seen[unsigned(quant+32)]=true;q6Cases++;negativeZero+=expected==0x80000000;
					if(fraction)printf("MIXED_FPU_FRACTION 6 %04x %d %d %08x\n",half[lane],scale[lane],quant,q6.lanes[0][lane]);
				}
			}
			uint32_t keep[8][32];
			for(unsigned lane=0;lane<32;lane++) {
				q8.lanes[0][lane]=batch*32+lane;
				for(unsigned reg:{1U,2U,4U,5U,6U,7U})keep[reg][lane]=q8.lanes[reg][lane];
			}
			q8.Execute(p.q8);assert(q8.okay);
			for(unsigned lane=0;lane<32;lane++) {
				unsigned raw=batch*32+lane;int quant=raw>=128?int(raw)-256:int(raw);
				uint32_t expected=MixedRound(PioneerFmaBits(PioneerHalfFloat(half[lane])*float(quant)));
				assert(q8.lanes[0][lane]==expected);
				for(unsigned reg:{1U,2U,4U,5U,6U,7U})assert(keep[reg][lane]==q8.lanes[reg][lane]);
				q8Seen[raw]=true;q8Cases++;minimumQ8+=quant==-128;negativeZero+=expected==0x80000000;
				if(fraction)printf("MIXED_FPU_FRACTION 8 %04x 1 %d %08x\n",half[lane],quant,q8.lanes[0][lane]);
			}
		}
	}
	assert(negativeZero && minimumQ8==trials);
	for(bool b:scaleSeen)assert(b);for(bool b:q6Seen)assert(b);for(bool b:q8Seen)assert(b);
	if(fraction){assert(q6Cases==32768 && q8Cases==8192);return 0;}
	unsigned finiteCount=0;for(unsigned h=0;h<65536;h++)if(((h>>10)&31)!=31){assert(halfSeen[h]);finiteCount++;}
	unsigned timingFaults=0;
	for(const MixedFpuWords* program:{&p.q6[0],&p.q6[1],&p.q6[2],&p.q6[3],&p.q8})
	for(unsigned drop=0;drop<program->words.size();drop++)if(program->words[drop]==0x8f000000) {
		MixedFpuMachine m;m.lanes[1][0]=0x3f800000;m.lanes[0][0]=17;m.lanes[3][0]=83;
		m.Execute(program==&p.q8?p.q8Prepare:p.q6Prepare);
		for(unsigned i=0;i<program->words.size();i++)if(i!=drop)m.Op(program->words[i]);
		(void)m.Read(0,0);assert(!m.okay);timingFaults++;
	}
	assert(timingFaults==10);
	printf("PASS OFFBOARD mixed FPU decode:Q6 %llu/Q8 %llu exact original-half/signed-scale/packed-quant BF16-RNE outputs;all%u finite half encodings/all256 signed scales/all64 Q6/all256 TC8 including-128;%u latency faults;register preservation/signed zeros;NO UNPACK/Dst/MOVD2A/GDDR/image/driver/hardware/model-quality/speed authority\n",(unsigned long long)q6Cases,(unsigned long long)q8Cases,finiteCount,timingFaults);
}
#endif
