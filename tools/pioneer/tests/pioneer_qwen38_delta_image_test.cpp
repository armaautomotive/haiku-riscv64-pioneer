/* SPDX-License-Identifier: MIT */
// Executes ACTUAL linked RV32IMF instructions,not the C++ implementation.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "../PioneerQwen38Hybrid.h"
#include "../PioneerQwen38DeltaImage.h"
struct DeltaMachine:ScalarMachine {
	unsigned stateWrites=0,rawWrites=0;
	explicit DeltaMachine(const char* path,unsigned trial):ScalarMachine(path,128,0) {
		memory.clear();
		memory[0x74000]=0x44383351; memory[0x74004]=128; memory[0x74008]=128; memory[0x7400c]=0;
		for (unsigned i=0;i<16384;i++) {
			memory[0x90000+i*4]=PioneerAttentionBits(trial?float(sin(i*.031+trial*.17)*.003):0);
			memory[0xa0000+i*4]=0xdeadbeef;
		}
		for (unsigned i=0;i<128;i++) {
			memory[0xb0000+i*4]=PioneerAttentionBits(trial?float(sin(i*.123+trial*.317)*.01):0);
			memory[0xb0200+i*4]=PioneerAttentionBits(trial?float(cos(i*.153+trial*.217)*.03):0);
			memory[0xb0400+i*4]=PioneerAttentionBits(trial?float(sin(i*.013+trial*.117)*2.1):0);
			memory[0xb0800+i*4]=0xdeadbeef;
		}
		memory[0xb0600]=PioneerAttentionBits(trial==2?0:trial==3?1:.83f);
		memory[0xb0604]=PioneerAttentionBits(trial==2?1:trial==3?0:.37f);
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
	}
	uint32_t Load(uint32_t a) override {
		assert(!(a&3));
		if (Range(a,0x64000,unsigned(code.size()))) return l1[Index(a)];
		assert(Range(a,0x74000,16) || Range(a,0x6e000,8192) || Range(a,0x90000,0x20000)
			|| Range(a,0xb0000,1544) || Range(a,0xb0800,512) || Range(a,0x17ffc0,12));
		assert(memory.count(a)); return memory.at(a);
	}
	void Store(uint32_t a,uint32_t v) override {
		assert(!(a&3));
		assert(Range(a,0x6e000,8192) || Range(a,0xa0000,65536)
			|| Range(a,0xb0800,512) || Range(a,0x17ffc0,12));
		if (Range(a,0x6e000,8192)) minStack=std::min(minStack,unsigned(a));
		if (Range(a,0xa0000,65536)) stateWrites++;
		if (Range(a,0xb0800,512)) rawWrites++;
		memory[a]=v;
	}
	bool RunDelta() {
		while (pc!=0x64038 && pc!=0x64058) StepScalar();
		assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
		if (pc==0x64058) { assert(memory.at(0x17ffc8)==0x464e4451 && !memory.at(0x17ffc4)); return false; }
		assert(memory.at(0x17ffc0)==0x6400c && memory.at(0x17ffc8)==0x314e4451);
		return memory.at(0x17ffc4)==128;
	}
	void Verify() {
		assert(stateWrites==16384 && rawWrites==128);
		double q[128],k[128],v[128],delta[128],next[16384];
		for (unsigned i=0;i<128;i++) {
			q[i]=PioneerAttentionFloat(memory.at(0xb0000+i*4)); k[i]=PioneerAttentionFloat(memory.at(0xb0200+i*4));
			v[i]=PioneerAttentionFloat(memory.at(0xb0400+i*4));
		}
		double decay=PioneerAttentionFloat(memory.at(0xb0600)),beta=PioneerAttentionFloat(memory.at(0xb0604));
		for (unsigned j=0;j<128;j++) {
			double predicted=0;
			for (unsigned i=0;i<128;i++) predicted+=double(PioneerAttentionFloat(memory.at(0x90000+(i*128+j)*4)))*decay*k[i];
			delta[j]=(v[j]-predicted)*beta;
		}
		for (unsigned i=0;i<128;i++) for (unsigned j=0;j<128;j++) {
			unsigned at=i*128+j;
			next[at]=double(PioneerAttentionFloat(memory.at(0x90000+at*4)))*decay+k[i]*delta[j];
			float actual=PioneerAttentionFloat(memory.at(0xa0000+at*4));
			assert(isfinite(actual) && fabs(actual-next[at])<1e-7*(1+fabs(next[at])));
		}
		for (unsigned j=0;j<128;j++) {
			double wanted=0; for (unsigned i=0;i<128;i++) wanted+=next[i*128+j]*q[i];
			float actual=PioneerAttentionFloat(memory.at(0xb0800+j*4));
			assert(isfinite(actual) && fabs(actual-wanted)<1e-7*(1+fabs(wanted)));
		}
	}
};
int main(int argc,char** argv)
{
	assert(argc==2);
	{
		auto image=Read((std::string(argv[1])+"/code.bin").c_str()),boot=Read((std::string(argv[1])+"/boot.bin").c_str());
		assert(image.size()==PIONEER_Q38_DELTA_IMAGE_WORDS*4 && boot.size()==4 && Word(boot,0)==PioneerQ38DeltaBoot);
		for (unsigned w=0;w<PIONEER_Q38_DELTA_CODE_CAPACITY;w++)
			assert(PioneerQ38DeltaCodeWord(w)==(w<image.size()/4?Word(image,w*4):0));
	}
	for (unsigned trial=0;trial<4;trial++) {
		DeltaMachine m(argv[1],trial); auto before=m.memory;
		assert(m.RunDelta()); m.Verify();
		for (const auto& p:before) if ((p.first>=0x90000 && p.first<0xa0000)
			|| (p.first>=0xb0000 && p.first<0xb0608)) assert(m.memory.at(p.first)==p.second);
		printf("OFFLINE actual Qwen3.8 GDN head trial%u steps%u stack%08x ALL state/output values PASS\n",trial,m.steps,m.minStack);
	}
	for (unsigned field=0;field<4;field++) for (unsigned bit=0;bit<32;bit++) {
		DeltaMachine m(argv[1],1); m.memory[0x74000+field*4]^=1U<<bit;
		assert(!m.RunDelta() && !m.stateWrites && !m.rawWrites);
	}
	for (uint32_t at:{0x90000U,0x9fffcU,0xb0000U,0xb01fcU,0xb0200U,0xb03fcU,
		0xb0400U,0xb05fcU,0xb0600U,0xb0604U}) for (uint32_t invalid:{0x7f800000U,0xff800000U,0x7fc00001U}) {
		DeltaMachine m(argv[1],1); m.memory[at]=invalid;
		assert(!m.RunDelta() && !m.stateWrites && !m.rawWrites);
	}
	for (uint32_t at:{0xb0600U,0xb0604U}) for (float invalid:{-.1f,1.1f}) {
		DeltaMachine m(argv[1],1); m.memory[at]=PioneerAttentionBits(invalid);
		assert(!m.RunDelta() && !m.stateWrites && !m.rawWrites);
	}
	puts("PASS OFFLINE actual RV32IMF Qwen3.8 GDN image/ALL descriptor bits/NaN+Inf+gate refusals/read-only prior state/strict memory whitelist;NO device/NoC/driver selection or performance claim");
}
