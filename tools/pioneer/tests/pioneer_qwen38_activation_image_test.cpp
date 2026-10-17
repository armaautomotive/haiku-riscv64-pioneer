/* SPDX-License-Identifier: MIT */
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "pioneer_qwen38_activation_reference.h"
#include "../PioneerQwen38ActivationImage.h"
struct ActivationMachine:ScalarMachine {
	unsigned writes=0;
	ActivationMachine(const char* path,unsigned trial):ScalarMachine(path,256,0) {
		memory.clear();
		memory[0x74000]=0x4b383351; memory[0x74004]=256; memory[0x74008]=292; memory[0x7400c]=0;
		for (unsigned i=0;i<256;i++) {
			float x=trial?float(int((i*317+trial*27)%20001)-10000)*.00137f:0;
			if (trial==2) x=(float(int(i%255)-127)+.5f)*.5f;
			if (trial>=2 && i<2) x=(i?1:-1)*(trial==2?-127.0f:127.0f);
			if (trial>=4) x=ldexpf(float(int((i*317+trial*27)%20001)-10000),int(trial-4)*8-110);
			memory[0x90000+i*4]=PioneerAttentionBits(x);
		}
		for (unsigned i=0;i<73;i++) memory[0x91000+i*4]=0xdeadbeef;
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
	}
	uint32_t Load(uint32_t a) override {
		assert(!(a&3)); if (Range(a,0x64000,unsigned(code.size()))) return l1[Index(a)];
		assert(Range(a,0x74000,16) || Range(a,0x6e000,8192) || Range(a,0x90000,1024)
			|| Range(a,0x91000,292) || Range(a,0x17ffc0,12));
		assert(memory.count(a)); return memory.at(a);
	}
	void Store(uint32_t a,uint32_t v) override {
		assert(!(a&3)); assert(Range(a,0x6e000,8192) || Range(a,0x91000,292) || Range(a,0x17ffc0,12));
		if (Range(a,0x6e000,8192)) minStack=std::min(minStack,unsigned(a));
		if (Range(a,0x91000,292)) writes++;
		memory[a]=v;
	}
	bool RunActivation() {
		while (pc!=0x64038 && pc!=0x64058) StepScalar();
		assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
		if (pc==0x64058) { assert(memory.at(0x17ffc8)==0x464b3851 && !memory.at(0x17ffc4)); return false; }
		assert(memory.at(0x17ffc0)==0x6400c && memory.at(0x17ffc8)==0x314b3851);
		return memory.at(0x17ffc4)==1;
	}
	void Verify() {
		float source[256]; uint32_t wanted[73];
		for (unsigned i=0;i<256;i++) source[i]=PioneerAttentionFloat(memory.at(0x90000+i*4));
		Q38ActivationReference(source,wanted); assert(writes==73);
		for (unsigned i=0;i<73;i++) assert(memory.at(0x91000+i*4)==wanted[i]);
	}
};
int main(int argc,char** argv)
{
	assert(argc==2);
	{
		auto image=Read((std::string(argv[1])+"/code.bin").c_str()),boot=Read((std::string(argv[1])+"/boot.bin").c_str());
		assert(image.size()==PIONEER_Q38_ACTIVATION_IMAGE_WORDS*4 && boot.size()==4 && Word(boot,0)==PioneerQ38ActivationBoot);
		for (unsigned w=0;w<PIONEER_Q38_ACTIVATION_CODE_CAPACITY;w++)
			assert(PioneerQ38ActivationCodeWord(w)==(w<image.size()/4?Word(image,w*4):0));
	}
	for (unsigned trial=0;trial<32;trial++) {
		ActivationMachine m(argv[1],trial); auto before=m.memory;
		assert(m.RunActivation()); m.Verify();
		for (const auto& p:before) if (p.first>=0x90000 && p.first<0x90400) assert(m.memory.at(p.first)==p.second);
		printf("OFFLINE actual Q8_K trial%u steps%u stack%08x ALL292 bytes PASS\n",trial,m.steps,m.minStack);
	}
	for (unsigned field=0;field<4;field++) for (unsigned bit=0;bit<32;bit++) {
		ActivationMachine m(argv[1],1); m.memory[0x74000+field*4]^=1U<<bit;
		assert(!m.RunActivation() && !m.writes);
	}
	for (unsigned i=0;i<256;i++) for (uint32_t invalid:{0x7f800000U,0xff800000U,0x7fc00001U,1U,0x80000001U}) {
		ActivationMachine m(argv[1],1); m.memory[0x90000+i*4]=invalid;
		assert(!m.RunActivation() && !m.writes);
	}
	{ ActivationMachine m(argv[1],1); for (unsigned i=0;i<256;i++) m.memory[0x90000+i*4]=0x00800000;
		assert(!m.RunActivation() && !m.writes); }
	puts("PASS OFFLINE actual RV32 Q8_K signed F32 scale/nearest-even/ALL bytes+sums/ALL descriptor bits/nonfinite+subnormal refusals/immutable input/strict memory;NO device execution");
}
