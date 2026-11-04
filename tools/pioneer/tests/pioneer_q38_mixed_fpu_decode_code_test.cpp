/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#define PIONEER_MIXED_FPU_MACHINE_ONLY
#include "pioneer_q38_mixed_fpu_decode_test.cpp"
#include <sstream>
// The existing strict RV32 Step decoder is UNCHANGED. No weight/data/NoC
// or config/Dst address exists;only code,private ABI stack,instruction port.
struct MixedFpuCodeMachine:Machine {
	std::vector<uint8_t> code;std::vector<uint32_t> issued;std::array<uint32_t,2048> stack{};
	unsigned portWrites=0,directIssues=0;bool direct;
	MixedFpuCodeMachine(std::vector<uint8_t> bytes,bool mode):code(std::move(bytes)),direct(mode) {
		assert(!code.empty() && code.size()<=4096 && !(code.size()&3));
		for(unsigned i=0;i<code.size()/4;i++)l1[i]=Word(code,i*4);
	}
	bool CodeAddress(uint32_t a)const override{return a>=0x70000 && a-0x70000<code.size() && !(a&3);}
	bool InlineTensix()const override{return direct;}
	void Instruction(uint32_t w)override{assert(direct);directIssues++;issued.push_back(w);}
	unsigned Index(uint32_t a)override{assert(CodeAddress(a));return (a-0x70000)/4;}
	uint32_t Load(uint32_t a)override{
		if(a>=0x6e000 && a<0x70000){assert(!(a&3));return stack[(a-0x6e000)/4];}
		assert(CodeAddress(a));return Word(code,a-0x70000);
	}
	void Store(uint32_t a,uint32_t v)override{
		if(a>=0x6e000 && a<0x70000){assert(!(a&3));stack[(a-0x6e000)/4]=v;return;}
		assert(a==0xffe40000);portWrites++;issued.push_back(v);
	}
};
int main(int argc,char** argv)
{
	assert(argc==4 && (!strcmp(argv[3],"mmio") || !strcmp(argv[3],"inline")));bool direct=!strcmp(argv[3],"inline");
	auto code=Read(argv[1]);std::ifstream symbols(argv[2]);assert(symbols.good());unsigned entry[7]={};std::string line;
	const char* names[]={"pioneer_q6_fpu_prepare","pioneer_q8_fpu_prepare","pioneer_q6_fpu_group0","pioneer_q6_fpu_group1","pioneer_q6_fpu_group2","pioneer_q6_fpu_group3","pioneer_q8_fpu_decode"};
	while(std::getline(symbols,line)) {
		std::istringstream in(line);std::string address,type,name;in>>address>>type>>name;
		for(unsigned i=0;i<7;i++)if(name==names[i]){assert(type=="T" && !entry[i]);entry[i]=unsigned(std::stoul(address,nullptr,16));}
	}
	MixedFpuPrograms expected,actual;MixedFpuWords* exp[]={&expected.q6Prepare,&expected.q8Prepare,&expected.q6[0],&expected.q6[1],&expected.q6[2],&expected.q6[3],&expected.q8};
	MixedFpuWords* out[]={&actual.q6Prepare,&actual.q8Prepare,&actual.q6[0],&actual.q6[1],&actual.q6[2],&actual.q6[3],&actual.q8};
	unsigned steps=0,total=0;
	for(unsigned i=0;i<7;i++) {
		assert(entry[i]);MixedFpuCodeMachine m(code,direct);m.pc=entry[i];m.regs[1]=0xdead0000;m.regs[2]=0x70000;
		for(unsigned r:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})m.regs[r]=0x5a000000|r;
		while(m.pc!=0xdead0000){assert(m.steps<512);m.Step();}
		assert(m.regs[2]==0x70000);
		for(unsigned r:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(m.regs[r]==(0x5a000000|r));
		assert(m.issued==exp[i]->words && m.portWrites==(direct?0U:m.issued.size()) && m.directIssues==(direct?m.issued.size():0U));
		out[i]->words=m.issued;steps+=m.steps;total+=unsigned(m.issued.size());
	}
	unsigned cases=0;
	for(unsigned trial=0;trial<256;trial++) {
		MixedFpuMachine q6,q8;uint16_t half[32];int scale[32];
		for(unsigned lane=0;lane<32;lane++) {
			half[lane]=uint16_t(((trial*331+lane*293)%0x7c00)|((lane&1)<<15));scale[lane]=int(trial)-128;
			uint32_t coefficient;assert(PioneerQ38BinaryScale(half[lane],scale[lane],coefficient));
			q6.lanes[1][lane]=coefficient;q8.lanes[1][lane]=PioneerFmaBits(PioneerHalfFloat(half[lane]));
		}
		q6.Execute(actual.q6Prepare);q8.Execute(actual.q8Prepare);
		for(unsigned first=0;first<256;first+=32) {
			for(unsigned group=0;group<4;group++) {
				for(unsigned lane=0;lane<32;lane++){q6.lanes[0][lane]=first+lane;q6.lanes[3][lane]=(trial+lane*19)&255;}
				q6.Execute(actual.q6[group]);assert(q6.okay);
				for(unsigned lane=0;lane<32;lane++) {
					unsigned raw=first+lane,high=(trial+lane*19)&255;
					int quant=int(((raw>>(group/2*4))&15)|(((high>>(group*2))&3)<<4))-32;
					float coefficient=PioneerHalfFloat(half[lane])*float(scale[lane]);
					assert(q6.lanes[0][lane]==MixedRound(PioneerFmaBits(coefficient*float(quant))));cases++;
				}
			}
			for(unsigned lane=0;lane<32;lane++)q8.lanes[0][lane]=first+lane;
			q8.Execute(actual.q8);assert(q8.okay);
			for(unsigned lane=0;lane<32;lane++) {
				int quant=int(first+lane);if(quant>=128)quant-=256;
				assert(q8.lanes[0][lane]==MixedRound(PioneerFmaBits(PioneerHalfFloat(half[lane])*float(quant))));cases++;
			}
		}
	}
	printf("PASS actual RV32I mixed FPU code:%s,seven functions,%u exact issued instructions,%u strict RISC steps,%u exact operand comparisons,ABI/callee registers preserved;NO scalar learned data/NoC/config/Dst/driver/hardware/quality/speed authority\n",direct?"inline-ZERO-port-writes":"MMIO-port",total,steps,cases);
}
