/* SPDX-License-Identifier: MIT */
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "../PioneerQwen38PackedDot.h"
#include "../PioneerQwen38PackedImage.h"
#include "../TenstorrentGgufQuant.h"
struct PackedMachine:ScalarMachine {
	unsigned type,outputWrites=0;
	uint8_t weights[210]={}; PioneerQ38Int8 activation[256]={};
	float scale=0;
	void Byte(uint32_t address,uint8_t byte) {
		unsigned shift=(address&3)*8; uint32_t& word=memory[address&~3U];
		word=(word&~(255U<<shift))|(uint32_t(byte)<<shift);
	}
	PackedMachine(const char* path,unsigned kind,unsigned trial):ScalarMachine(path,128,0),type(kind) {
		memory.clear();
		unsigned bytes=type==8?34:type==12?144:210,n=type==8?32:256;
		memory[0x74000]=0x50383351; memory[0x74004]=type; memory[0x74008]=bytes; memory[0x7400c]=n;
		for (unsigned i=0;i<210;i++) weights[i]=uint8_t(i*37+trial*59);
		unsigned d=type==14?208:0; weights[d]=0; weights[d+1]=0x3c;
		if (type==12) { weights[2]=0; weights[3]=0x38; }
		for (unsigned i=0;i<256;i++) {
			activation[i]=trial?PioneerQ38Int8(int((i*37+trial*17)%256)-128):0;
			Byte(0x90000+i,i<210?weights[i]:0); Byte(0x91000+i,uint8_t(activation[i]));
		}
		scale=trial==2?-.015625f:.03125f; memory[0x92000]=PioneerAttentionBits(scale); memory[0x93000]=0xdeadbeef;
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
	}
	uint32_t Load(uint32_t a) override {
		assert(!(a&3)); if (Range(a,0x64000,unsigned(code.size()))) return l1[Index(a)];
		assert(Range(a,0x74000,16) || Range(a,0x6e000,8192) || Range(a,0x90000,256)
			|| Range(a,0x91000,256) || a==0x92000 || a==0x93000 || Range(a,0x17ffc0,12));
		assert(memory.count(a)); return memory.at(a);
	}
	void Store(uint32_t a,uint32_t v) override {
		assert(!(a&3)); assert(Range(a,0x6e000,8192) || a==0x93000 || Range(a,0x17ffc0,12));
		if (Range(a,0x6e000,8192)) minStack=std::min(minStack,unsigned(a));
		if (a==0x93000) outputWrites++;
		memory[a]=v;
	}
	bool RunPacked() {
		while (pc!=0x64038 && pc!=0x64058) StepScalar();
		assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
		if (pc==0x64058) { assert(memory.at(0x17ffc8)==0x464b5051 && !memory.at(0x17ffc4)); return false; }
		assert(memory.at(0x17ffc0)==0x6400c && memory.at(0x17ffc8)==0x314b5051);
		return memory.at(0x17ffc4)==1;
	}
	void Verify() {
		float decoded[256]={},actual=PioneerAttentionFloat(memory.at(0x93000));
		assert(outputWrites==1 && isfinite(actual));
		bool good=type==8?PioneerDecodeQ8_0(weights,decoded):type==12?PioneerDecodeQ4K(weights,decoded):PioneerDecodeQ6K(weights,decoded);
		assert(good); double wanted=0,absolute=0;
		for (unsigned i=0;i<(type==8?32U:256U);i++) { double value=double(decoded[i])*activation[i]*scale; wanted+=value; absolute+=fabs(value); }
		assert(fabs(actual-wanted)<1e-6+absolute*5e-7);
	}
};
int main(int argc,char** argv)
{
	assert(argc==2);
	{
		auto image=Read((std::string(argv[1])+"/code.bin").c_str()),boot=Read((std::string(argv[1])+"/boot.bin").c_str());
		assert(image.size()==PIONEER_Q38_PACKED_IMAGE_WORDS*4 && boot.size()==4 && Word(boot,0)==PioneerQ38PackedBoot);
		for (unsigned w=0;w<PIONEER_Q38_PACKED_CODE_CAPACITY;w++)
			assert(PioneerQ38PackedCodeWord(w)==(w<image.size()/4?Word(image,w*4):0));
	}
	for (unsigned type:{8U,12U,14U}) {
		for (unsigned trial=0;trial<3;trial++) {
			PackedMachine m(argv[1],type,trial); auto before=m.memory;
			assert(m.RunPacked()); m.Verify();
			for (const auto& p:before) if ((p.first>=0x90000 && p.first<0x92000) || p.first==0x92000) assert(m.memory.at(p.first)==p.second);
			printf("OFFLINE actual packed type%u trial%u steps%u stack%08x PASS\n",type,trial,m.steps,m.minStack);
		}
		for (unsigned field=0;field<4;field++) for (unsigned bit=0;bit<32;bit++) {
			PackedMachine m(argv[1],type,1); m.memory[0x74000+field*4]^=1U<<bit;
			assert(!m.RunPacked() && !m.outputWrites);
		}
		for (uint32_t invalid:{0x7f800000U,0xff800000U,0x7fc00001U}) {
			PackedMachine m(argv[1],type,1); m.memory[0x92000]=invalid; assert(!m.RunPacked() && !m.outputWrites);
		}
		{ PackedMachine m(argv[1],type,1); unsigned d=type==14?208:0;
			m.Byte(0x90000+d,1); m.Byte(0x90001+d,0x7c); assert(!m.RunPacked() && !m.outputWrites); }
		if (type==12) { PackedMachine m(argv[1],type,1); m.Byte(0x90002,1); m.Byte(0x90003,0x7c); assert(!m.RunPacked() && !m.outputWrites); }
	}
	puts("PASS OFFLINE actual RV32 packed Q4_K/Q6_K/Q8_0 integer reductions/signed scales/min correction/ALL descriptor bits/nonfinite refusal/immutable inputs/strict memory;NOT a fast Tensix matrix backend");
}
