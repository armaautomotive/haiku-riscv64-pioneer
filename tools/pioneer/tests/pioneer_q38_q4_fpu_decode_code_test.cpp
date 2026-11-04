/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#define PIONEER_Q38_Q4_FPU_DECODE_MODEL_ONLY
#include "pioneer_q38_q4_fpu_decode_test.cpp"
#include <sstream>
// Actual RV32I program bytes executed by the unchanged strict Step decoder.
// Only code/private ABI stack and the instruction port are addressable.
struct Q4FpuCodeMachine:Machine {
	std::vector<uint8_t> code;std::vector<uint32_t> issued;
	std::array<uint32_t,2048> stack{};unsigned portWrites=0,directIssues=0;bool direct;
	Q4FpuCodeMachine(std::vector<uint8_t> bytes,bool inlineMode):code(std::move(bytes)),direct(inlineMode) {
		assert(code.size()<=4096 && !(code.size()&3));
		for(unsigned i=0;i<code.size()/4;i++)l1[i]=Word(code,i*4);
	}
	bool CodeAddress(uint32_t address) const override{return address>=0x70000 && address-0x70000<code.size() && !(address&3);}
	bool InlineTensix() const override{return direct;}
	void Instruction(uint32_t word) override{assert(direct);directIssues++;issued.push_back(word);}
	unsigned Index(uint32_t address) override{assert(CodeAddress(address));return (address-0x70000)/4;}
	uint32_t Load(uint32_t address) override {
		if(address>=0x6e000 && address<0x70000){assert(!(address&3));return stack[(address-0x6e000)/4];}
		assert(address>=0x70000 && address-0x70000<code.size());return Word(code,address-0x70000);
	}
	void Store(uint32_t address,uint32_t value) override {
		if(address>=0x6e000 && address<0x70000){assert(!(address&3));stack[(address-0x6e000)/4]=value;return;}
		assert(address==0xffe40000);portWrites++;issued.push_back(value);
	}
};
int main(int argc,char** argv)
{
	assert(argc==4 && (!strcmp(argv[3],"mmio") || !strcmp(argv[3],"inline")));const bool direct=!strcmp(argv[3],"inline");
	const auto code=Read(argv[1]);assert(!code.empty() && code.size()<=4096);
	std::ifstream symbols(argv[2]);assert(symbols.good());unsigned entry[3]={};std::string line;
	const char* names[]={"pioneer_q4_fpu_prepare","pioneer_q4_fpu_low","pioneer_q4_fpu_high"};
	while(std::getline(symbols,line)) {
		std::istringstream input(line);std::string address,type,name;input>>address>>type>>name;
		for(unsigned i=0;i<3;i++)if(name==names[i]){assert(type=="T" && !entry[i]);entry[i]=unsigned(std::stoul(address,nullptr,16));}
	}
	Q4FpuWords expected[3],actual[3];PioneerQ38Q4FpuPrepare(expected[0]);
	PioneerQ38Q4FpuDecode<Q4FpuWords,false>(expected[1]);PioneerQ38Q4FpuDecode<Q4FpuWords,true>(expected[2]);
	unsigned steps=0,total=0;
	for(unsigned function=0;function<3;function++) {
		assert(entry[function]);Q4FpuCodeMachine m(code,direct);m.pc=entry[function];m.regs[1]=0xdead0000;m.regs[2]=0x70000;
		for(unsigned reg:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})m.regs[reg]=0x5a000000|reg;
		while(m.pc!=0xdead0000){assert(m.steps<512);m.Step();}
		assert(m.regs[2]==0x70000);
		for(unsigned reg:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(m.regs[reg]==(0x5a000000|reg));
		assert(m.issued==expected[function].words);
		assert(m.portWrites==(direct?0U:m.issued.size()) && m.directIssues==(direct?m.issued.size():0U));
		actual[function].words=m.issued;steps+=m.steps;total+=unsigned(m.issued.size());
	}
	unsigned cases=0;
	for(unsigned trial=0;trial<64;trial++) {
		Q4FpuMachine lanes;uint16_t half[32],minimum[32];
		for(unsigned lane=0;lane<32;lane++) {
			half[lane]=uint16_t(((trial*431+lane*991)%0x7c00)|((lane&1)<<15));
			minimum[lane]=uint16_t(((trial*199+lane*123)%0x7c00)|((lane&2)<<14));
			lanes.lanes[1][lane]=PioneerFmaBits(PioneerHalfFloat(half[lane]));
			lanes.lanes[2][lane]=PioneerFmaBits(PioneerHalfFloat(minimum[lane]));
			lanes.lanes[3][lane]=trial;lanes.lanes[4][lane]=(trial+lane)&63;
		}
		lanes.Execute(actual[0]);
		for(unsigned first=0;first<256;first+=32)for(unsigned upper=0;upper<2;upper++) {
			for(unsigned lane=0;lane<32;lane++)lanes.lanes[0][lane]=first+lane;
			lanes.Execute(actual[upper+1]);assert(lanes.okay);
			for(unsigned lane=0;lane<32;lane++) {
				const float coefficient=PioneerHalfFloat(half[lane])*float(trial);
				const float subtract=PioneerHalfFloat(minimum[lane])*float((trial+lane)&63);
				const float product=coefficient*float(((first+lane)>>(upper*4))&15),value=product-subtract;
				assert(lanes.lanes[0][lane]==Round(PioneerFmaBits(value)));cases++;
			}
		}
	}
	printf("PASS actual RV32I compressed-Q4 FPU code:%s,3 functions,%u issued instructions,%u strict RISC steps,%u exact operand comparisons,private ABI stack/callee registers preserved;NO scalar weight/activation/NoC/config/Dst/unpack/driver/hardware/quality/speed authority\n",direct?"inline-ZERO-port-writes":"MMIO-port",total,steps,cases);
}
