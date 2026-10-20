/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_BLOCK_MACHINE_ONLY
#include "pioneer_qwen38_block_image_test.cpp"
#define PIONEER_Q38_OUTPUT_TRANSFER_MODEL_ONLY
#include "pioneer_q38_output_transfer_test.cpp"
struct OutputMachine:BlockMachine {
	Fabric fabric;
	bool changeLiveDescriptor=false;
	OutputMachine(const char* image,unsigned peers,bool edge=false)
		:BlockMachine(image,8,0,.01f,nullptr,true),fabric(peers,edge) {
		for (unsigned i=0;i<32;i++) memory[0x74000+i*4]=fabric.d[i];
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0xffb20000,0x2000)) return fabric.ReadReg(a-0xffb20000);
		if (Range(a,0x80000,fabric.d[7]) || Range(a,0x75000,4096)) return fabric.ReadL1(a);
		if (CodeAddress(a)) return BlockMachine::Load(a);
		assert(!(a&3) && (Range(a,0x6e000,8192) || Range(a,0x74000,128) || Range(a,0x17ffc0,12)));
		return memory.at(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0xffb20000,0x2000)) {
			fabric.WriteReg(a-0xffb20000,value);
			if (changeLiveDescriptor && fabric.packets==1) memory[0x74010]=fabric.d[4]^0x100000;
			return;
		}
		assert(!(a&3) && (Range(a,0x6e000,8192) || Range(a,0x17ffc0,12)));
		if (Range(a,0x6e000,8192)) minStack=std::min(minStack,a);
		memory[a]=value;
	}
	bool Run() {
		while (pc!=0x72038 && pc!=0x7207c) StepBlock();
		assert(!matrixInstructions && !moveInstructions && minStack>=0x6e000);
		if (pc==0x7207c) {
			unsigned old=fabric.io;
			for (unsigned i=0;i<100;i++) StepBlock();
			assert(fabric.io==old); return false;
		}
		assert(memory.at(0x17ffc8)==0x314d3851);
		return memory.at(0x17ffc4)==fabric.d[2];
	}
};
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned peers:{1U,2U,4U,7U,119U}) for (bool edge:{false,true}) {
		OutputMachine m(argv[1],peers,edge); assert(m.Run());
		assert(m.fabric.host==m.fabric.source && m.fabric.posted.empty());
	}
	for (unsigned packet:{1U,30U,60U}) for (unsigned fault=0;fault<4;fault++) {
		if ((fault==2 && !(packet&1)) || (fault==3 && (packet&1))) continue;
		OutputMachine m(argv[1],119);
		if (!fault) m.fabric.fail=packet; else if (fault==1) m.fabric.badCounter=packet;
		else if (fault==2) m.fabric.dropWrite=packet; else m.fabric.corruptRead=packet;
		assert(!m.Run() && m.fabric.packets==(fault==2?packet+1:packet));
	}
	for (unsigned field=0;field<16;field++) {
		OutputMachine m(argv[1],7); if (field==3) m.memory[0x74000+field*4]=0;
		else m.memory[0x74000+field*4]^=1;
		assert(!m.Run() && !m.fabric.io && !m.fabric.packets);
	}
	for (unsigned word=1;word<=10;word++) {
		OutputMachine m(argv[1],119); m.fabric.dropControl=word;
		assert(!m.Run() && !m.fabric.packets);
	}
	OutputMachine changed(argv[1],119); changed.changeLiveDescriptor=true;
	assert(!changed.Run() && changed.fabric.host==changed.fabric.source);
	puts("PASS OFFLINE ACTUAL RV32 bounded output image: owned-window-only reads/writes, high48-bit host, all raw bits, delayed host writes/full ordered readback, packet uncertainty and changed descriptor fault-park with ZERO later NoC IO;NOT kernel/native launch authority");
}
