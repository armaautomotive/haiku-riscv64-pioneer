/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_RAW_MACHINE_ONLY
#include "pioneer_q38_native_raw_operand_test.cpp"
#include <array>
#include <sstream>
struct CodeMachine:Machine {
	std::vector<uint8_t> code;std::vector<uint32_t> issued;
	std::array<uint32_t,2048> stack{};unsigned stackReads=0,stackWrites=0,portWrites=0,directIssues=0;bool direct;
	explicit CodeMachine(std::vector<uint8_t> bytes,bool inlineMode):code(std::move(bytes)),direct(inlineMode) {
		assert(code.size()<=4096 && !(code.size()&3));
		// Step fetches l1[Index(pc)] directly, not virtual Load(pc).
		// Replace the base fixture's resident program with ACTUAL probe bytes.
		for(unsigned index=0;index<code.size()/4;index++)l1[index]=Word(code,index*4);
	}
	bool CodeAddress(uint32_t address) const override {return address>=0x70000 && address-0x70000<code.size() && !(address&3);}
	bool InlineTensix() const override {return direct;}
	void Instruction(uint32_t word) override {assert(direct);directIssues++;issued.push_back(word);}
	unsigned Index(uint32_t address) override {assert(CodeAddress(address));return (address-0x70000)/4;}
	uint32_t Load(uint32_t address) override {
		if(address>=0x6e000 && address<0x70000){assert(!(address&3));stackReads++;return stack[(address-0x6e000)/4];}
		assert(address>=0x70000 && address-0x70000<code.size());return Word(code,address-0x70000);
	}
	void Store(uint32_t address,uint32_t value) override {
		if(address>=0x6e000 && address<0x70000){assert(!(address&3));stackWrites++;stack[(address-0x6e000)/4]=value;return;}
		if(address!=0xffe40000)fprintf(stderr,"RAW_CODE_UNEXPECTED_STORE pc%08x address%08x value%08x sp%08x\n",pc,address,value,regs[2]);
		assert(address==0xffe40000);portWrites++;issued.push_back(value);
	}
};
struct WordEmitter {std::vector<uint32_t> words;void Op(uint32_t word){words.push_back(word);}};
int main(int argc,char** argv) {
	assert(argc==4 && (!strcmp(argv[3],"mmio") || !strcmp(argv[3],"inline")));const bool direct=!strcmp(argv[3],"inline");
	const auto text=Read(argv[1]);assert(!text.empty() && text.size()<=4096);
	std::ifstream file(argv[2]);assert(file.good());std::string line;unsigned entries[3]={};
	while(std::getline(file,line)) {
		std::istringstream input(line);std::string address,type,name;input>>address>>type>>name;
		for(unsigned i=0;i<3;i++)if(name==std::string("pioneer_native_raw")+char('0'+(2U<<i))) {
			assert(type=="T" && !entries[i]);entries[i]=unsigned(std::stoul(address,nullptr,16));
		}
	}
	unsigned cases=0,steps=0;
	for(unsigned variant=0;variant<3;variant++) {
		const unsigned bits=2U<<variant;assert(entries[variant]);
		CodeMachine code(text,direct);code.pc=entries[variant];code.regs[1]=0xdead0000;code.regs[2]=0x70000;
		for(unsigned reg:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})code.regs[reg]=0x5a000000|reg;
		while(code.pc!=0xdead0000){assert(code.steps<512);code.Step();}
		assert(code.regs[2]==0x70000);
		for(unsigned reg:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(code.regs[reg]==(0x5a000000|reg));
		WordEmitter expected;assert(PioneerQ38NativeRawRecover(expected,bits));assert(code.issued==expected.words && code.issued.size()==9);
		assert(code.portWrites==(direct?0U:9U) && code.directIssues==(direct?9U:0U));steps+=code.steps;
		RawMachine lanes;
		for(unsigned first=0;first<(1U<<bits);first++) {
			for(unsigned lane=0;lane<32;lane++)lanes.lanes[0][lane]=UnpackBits(bits,(first+lane)% (1U<<bits));
			for(uint32_t word:code.issued)lanes.Instruction(word);
			for(unsigned lane=0;lane<32;lane++)assert(lanes.lanes[0][lane]==(first+lane)% (1U<<bits));
			if(bits==4 || bits==8) {
				RawEmitter emit{lanes};assert(PioneerQ38VectorOperand(emit,bits==4?12:8,0));
				for(unsigned lane=0;lane<32;lane++) {int value=int((first+lane)% (1U<<bits));if(bits==8 && value>=128)value-=256;CheckOverlay(lanes.lanes[0][lane],value);}
			}
			cases+=32;
		}
	}
	printf("PASS actual RV32I raw-operand code:%s,3 functions,strict unchanged Step,bounded private8KiB ABI stack/callee-register preservation,exact9 SFPU commands each,%u all-domain/lane recoveries,%u total RISC steps;NO scalar weight/activation access or FP dequant,unpack/Dst scheduling/boot/owner/driver/hardware/speed qualification\n",direct?"inline-ZERO-port-writes":"MMIO-port",cases,steps);
}
