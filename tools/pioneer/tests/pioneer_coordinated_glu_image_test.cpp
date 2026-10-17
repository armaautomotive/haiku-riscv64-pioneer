/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_COORDINATED_GROUP_MACHINE_PROVIDED
#define PIONEER_COORDINATED_ATTENTION_MACHINE_ONLY
#include "pioneer_coordinated_attention_image_test.cpp"
#endif
#include "../PioneerGraphMath.h"
#include "../PioneerCoordinatedGluProof.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
struct GluMachine:GroupMachine {
	unsigned operandWrites=0,scaleWrites=0;
	GluMachine(const Image& image,unsigned l,unsigned g):GroupMachine(image,1,l,g) {
		mem[0x74000/4]=mem[0x7051c/4]=0x314c4741; mem[0x74004/4]=512;
		for (unsigned i=0;i<512;i++) { mem[0x90000/4+i]=B(float(sin(i*.193+g*.371)*15)); mem[0x90800/4+i]=B(float(cos(i*.417+l*.137)*3.1)); }
		if (image.inputPull) for (unsigned i=0;i<512;i++) {
			masterInput[0x87000+g*2048+i*4]=mem[0x90000/4+i]; masterInput[0x84000+g*2048+i*4]=mem[0x90800/4+i];
			mem[0x90000/4+i]=mem[0x90800/4+i]=0xcafebabe;
		}
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0x90000,8192)) return mem[a/4];
		return GroupMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		assert((image.vectorGlu || a!=0xffe40000) && (image.inputPull || !Range(a,0xffb20000,sizeof(noc)))); // SFPU only in explicit vector image;NoC only in explicit peer-pull image.
		if (Range(a,0x91000,1024) || Range(a,0x91400,64) || Range(a,0x91500,128)) {
			if (Range(a,0x91000,1024)) operandWrites++; if (Range(a,0x91400,64)) scaleWrites++;
			mem[a/4]=value; return;
		}
		GroupMachine::Store(a,value);
	}
	void VerifyGlu() {
		assert(mem[0x17ffc4/4]==512 && commands==unsigned(image.inputPull?4:0) && (image.vectorGlu?mmioWrites>0:!mmioWrites) && operandWrites==256 && scaleWrites==16);
		for (unsigned block=0;block<16;block++) {
			float values[32],maximum=0;
			for (unsigned i=0;i<32;i++) { unsigned at=block*32+i; values[i]=PioneerGraphSiluFast(F(mem[0x90000/4+at]))*F(mem[0x90800/4+at]); maximum=fmaxf(maximum,fabsf(values[i])); }
			float d=maximum/127.0f,inverse=d?1.0f/d:0; _Float16 half=_Float16(d); assert(mem[0x91400/4+block]==B(float(half)));
			for (unsigned i=0;i<16;i++) { int a=int(roundf(values[i*2]*inverse)),b=int(roundf(values[i*2+1]*inverse));
				uint32_t expected=uint32_t(PioneerInt8Operand(a))|(uint32_t(PioneerInt8Operand(b))<<16);
				assert(mem[0x91000/4+block*16+i]==expected); }
		}
		uint32_t proof[32]; for (unsigned i=0;i<32;i++) proof[i]=mem[0x91500/4+i]; assert(PioneerCoordinatedGluProof(proof,layer,group,image.inputPull));
		for (unsigned i=0;i<32;i++) { proof[i]^=1; assert(!PioneerCoordinatedGluProof(proof,layer,group,image.inputPull)); proof[i]^=1; }
	}
};
#ifndef PIONEER_COORDINATED_GLU_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2); setvbuf(stdout,NULL,_IONBF,0); Image image(argv[1],false,true,true);
	for (unsigned layer:{0U,27U}) for (unsigned group=0;group<6;group++) {
		GluMachine m(image,layer,group); m.Run(); m.VerifyGlu(); printf("OFFLINE actual GLU/Q8 group%u layer%u 512 elements/exact packed operands/scales/source-proof metadata PASS\n",group,layer);
	}
	for (unsigned fault=0;fault<12;fault++) { GluMachine m(image,0,0);
		if (!fault) m.mem[0x74004/4]=0; else if (fault==1) m.mem[0x74008/4]=28; else if (fault==2) m.mem[0x7400c/4]=6;
		else if (fault==3) m.mem[0x7051c/4]=0; else if (fault<10) m.mem[0x70500/4+fault-4]^=1;
		else if (image.inputPull) m.masterInput[fault==10?0x87000:0x84000]=0x7fc00000;
		else m.mem[(fault==10?0x90000:0x90800)/4]=0x7fc00000;
		m.Run(); assert(!m.mem[0x17ffc4/4] && !m.operandWrites && !m.scaleWrites && m.commands==unsigned(image.inputPull && fault>=10?4:0)); }
	// The very same extended image must still run the original KV group.
	if (image.inputPull) {
		for (unsigned word=1;word<=10;word++) { GluMachine m(image,27,5); m.badCommandWord=word; m.Run(true);
			assert(m.commandInjected && !m.commands && !m.mem[0x17ffc4/4] && !m.operandWrites); }
		for (unsigned packet=1;packet<=4;packet++) for (unsigned fault=0;fault<6;fault++) { GluMachine m(image,27,5);
			if (fault<4) { m.badCounter=packet; m.badCounterWord=fault+1; } else if (fault==4) m.corruptPull=packet; else m.failCommand=packet;
			m.Run(true); assert(!m.mem[0x17ffc4/4] && !m.operandWrites); }
	}
	{ GroupMachine m(image,1,27,7); std::vector<float> q(256,1),expected(256); std::vector<uint16_t> k(128,0x3c00),v(128,0x4000);
		m.Inputs(q,k,v); Reference(q,k,v,1,expected); m.Run(); m.Verify(); assert(m.Error(expected)<.00002); }
	puts("PASS OFFLINE actual dual attention/GLU image,all6 GLU groups,legacy KV branch,exact original SiLU/Q8 packing,wrapped zero-NoC proof,bad descriptor/data refusal;NOT concurrent/native qualification");
}
#endif
