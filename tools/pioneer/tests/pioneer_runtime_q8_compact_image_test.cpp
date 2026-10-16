/* SPDX-License-Identifier: MIT */
#define PIONEER_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8CompactImage.h"
struct CompactQ8Machine:Q8Machine {
	uint32_t tail[1024]={};
	unsigned expansions=0;
	bool dropExpansion=false,corruptScale=false;
	CompactQ8Machine(unsigned k,unsigned n,unsigned t,uint32_t address=0x04000000):Q8Machine(k,n,t) {
		assert(PioneerQ8CompactShape(k,n)); base=target=address;
		for (unsigned w=0;w<1024;w++) l1[1+w]=BlackholeRuntimeQ8CompactCodeWord(w);
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=k/16; l1[Index(0x70508)]=n/16;
		weights.assign(size_t(PioneerQ8CompactWeightBytes(k,n)/4),0);
		for (unsigned row=0;row<n;row++) for (unsigned block=0;block<k/32;block++) {
			uint64_t byte=PioneerQ8CompactScaleByte(k,block,row);
			weights[size_t(byte/4)]|=uint32_t(WeightScale(row,block))<<(8*(byte&3));
			for (unsigned i=0;i<32;i++) {
				byte=PioneerQ8CompactValueByte(k,block*32+i,row);
				weights[size_t(byte/4)]|=uint32_t(uint8_t(Weight(row,block*32+i)))<<(8*(byte&3));
			}
		}
		for (unsigned w=0;w<1024;w++) tail[w]=0xfeedface;
	}
	bool CodeAddress(uint32_t a) const override { return a<0x70e00 && Q8Machine::CodeAddress(a); }
	void Store(uint32_t a,uint32_t v) override {
		if (a==0xffb20040) assert(!(regs[20]&1)); // never submit twice for a Q8 block
		Q8Machine::Store(a,v);
	}
	void Store16(uint32_t a,uint16_t v) override {
		assert(drained && !pending && !(a&1) && a>=0x70710 && a<0x70910);
		assert(PioneerInt8OperandValid(v)); expansions++;
		if (dropExpansion) return;
		unsigned w=Index(a&~3U),shift=8*(a&3);
		l1[w]=(l1[w]&~(65535U<<shift))|(uint32_t(v)<<shift);
	}
	uint32_t Load(uint32_t a) override {
		if (a>=0x17e000 && a<0x17f000) { assert(!pending); return tail[(a-0x17e000)/4]; }
		if (a>=0x70e00 && a<0x71000) return l1[Index(a)];
		if (a==0xffb20040) {
			doorbellRead=true;
			if (pending && ++waits>delay && requests!=faultAt) {
				for (unsigned w=0;w<144;w++) tail[w]=weights[(requests-1)*144+w]^((corrupt && w==8)?0x01010101:0)^((corruptScale && w==0)?0x04000400:0);
				pending=false; responses++;
			}
			return pending?1:0;
		}
		return Q8Machine::Load(a);
	}
};
#ifndef PIONEER_COMPACT_Q8_MACHINE_ONLY
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	for (unsigned w=0;w<1024;w++) assert(Word(code,w*4)==BlackholeRuntimeQ8CompactCodeWord(w));
	// Verify every immutable lookup entry independently, including signed-byte
	// boundaries, before trusting its role in executing the new RISC loads.
	for (unsigned q=0;q<256;q++) {
		uint16_t actual=uint16_t(Word(code,0xe00+(q/2)*4)>>(16*(q&1)));
		int signedByte=q<128?int(q):int(q)-256;
		assert(actual==PioneerInt8Operand(signedByte));
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) for (unsigned t=0;t<2;t++) {
		CompactQ8Machine m(k,32,t); m.delay=3; m.Run();
		assert(m.CorrectQ8() && m.requests==m.ExpectedFixtures()/2 && m.responses==m.requests);
		assert(m.expansions==256*m.ExpectedFixtures() && m.matrixInstructions==4*m.ExpectedFixtures());
		assert(m.casts==2*(k/32)*2 && m.mads==4*(k/32)*2 && m.scales==32*(k/32)*2);
		assert(PioneerQ8CompactWeightBytes(k,32)*2==PioneerMatvecWeightBytes(k,32));
		assert(!(PioneerQ8CompactWeightBytes(k,32)&4095));
	}
	CompactQ8Machine alternate(1024,64,1,0x04800000); alternate.Run(); assert(alternate.CorrectQ8());
	for (unsigned fault=0;fault<6;fault++) {
		CompactQ8Machine m(1024,32,1); m.omitCast=fault==0; m.omitScale=fault==1; m.omitAccumulate=fault==2;
		m.dropExpansion=fault==3; m.corrupt=fault==4; m.corruptScale=fault==5;
		m.Run(); assert(!m.CorrectQ8());
	}
	for (unsigned fault:{1U,33U,64U}) {
		CompactQ8Machine m(1024,32,0); m.faultAt=fault; bool failed=false;
		try { m.Run(); } catch(uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && m.requests==fault && m.responses==fault-1 && m.expansions==512*(fault-1));
	}
	assert(!PioneerQ8CompactShape(1024,16) && !PioneerQ8CompactShape(1024,48) && !PioneerQ8CompactShape(512,32));
	assert(PioneerQ8CompactWeightRange(40960,4096,1024,32) && !PioneerQ8CompactWeightRange(36864,4096,1024,32));
	assert(!PioneerQ8CompactWeightRange(40960,UINT64_MAX-4095,1024,32));
	puts("PASS: compact Q8 image, lossless signed-byte table, all K/N loops, half GDDR bytes/NoC requests, independent scaled references and corruption/fault rejection; NO hardware");
}
#endif
