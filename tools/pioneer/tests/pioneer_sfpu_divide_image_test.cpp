/* SPDX-License-Identifier: MIT */
#define main PioneerAttentionOriginalTests
#include "pioneer_attention_image_test.cpp"
#undef main
#include "../PioneerSfpuDivide.h"
struct DivideProbeMachine:Machine {
	unsigned dstWrites=0,dropDst=0;
	DivideProbeMachine(const Image& i):Machine(i,32) {
		mem[0x74000/4]=0x31564453; mem[0x74004/4]=32; mem[0x74008/4]=mem[0x7400c/4]=0;
		for (unsigned lane=0;lane<32;lane++) { mem[0x90000/4+lane]=mem[0x90800/4+lane]=B(1); mem[0x91000/4+lane]=0xdeadbeef; }
		mem[0x90ffc/4]=mem[0x91080/4]=0x123cafe;
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0x90000,128) || Range(a,0x90800,128) || Range(a,0x91000,128)) return mem[a/4];
		assert(a==0xffe000fc || Range(a,0xffef0000,224*4) || Range(a,0xffbd8000,12*64)
			|| Range(a,0x72000,unsigned(image.code.size())) || Range(a,0x74000,16) || Range(a,0x76000,8192) || Range(a,0x17ffc0,12));
		return Machine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0x91000,128)) { assert(drained); outputWrites++; mem[a/4]=value; return; }
		assert(a==0xffe40000 || a==0xffe000fc || Range(a,0xffef0000,224*4) || Range(a,0xffbd8000,8*64)
			|| Range(a,0x76000,8192) || Range(a,0x17ffc0,12)); // NO input/code/NoC/PCI/reset writes
		if (Range(a,0xffbd8000,8*64) && ++dstWrites==dropDst) return;
		Machine::Store(a,value);
	}
	void Verify(const uint32_t* expected) {
		assert(mem[0x17ffc4/4]==32 && outputWrites==32 && dstWrites==64 && !conditional && !mads && drains==3);
		for (unsigned lane=0;lane<32;lane++) assert(mem[0x91000/4+lane]==expected[lane]);
		assert(mem[0x90ffc/4]==0x123cafe && mem[0x91080/4]==0x123cafe);
	}
};
int main(int argc,char** argv)
{
	(void)PioneerAttentionOriginalTests;
	assert(argc==2); setenv("PIONEER_SFPU_DIVIDE_PROBE","1",1); Image image(argv[1]); uint32_t seed=0x36b31aa9;
	auto next=[&]() { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; };
	unsigned maximumSteps=0,maximumStack=0;
	for (unsigned trial=0;trial<1024;trial++) {
		DivideProbeMachine m(image); uint32_t expected[32],before[64];
		for (unsigned lane=0;lane<32;lane++) {
			uint32_t a,b; do { a=next()&0x7fffffff; b=next()&0x7fffffff; } while (!PioneerSfpuDivideDomain(a,b));
			if (trial<253) { a=(trial+1)<<23; b=0x3f800000; }
			m.mem[0x90000/4+lane]=before[lane]=a; m.mem[0x90800/4+lane]=before[32+lane]=b; expected[lane]=B(F(a)/F(b));
		}
		m.Run(); m.Verify(expected); maximumSteps=std::max(maximumSteps,m.steps); maximumStack=std::max(maximumStack,0x78000-m.minStack);
		for (unsigned lane=0;lane<32;lane++) assert(m.mem[0x90000/4+lane]==before[lane] && m.mem[0x90800/4+lane]==before[32+lane]);
	}
	for (unsigned word=1;word<=64;word++) { DivideProbeMachine m(image); m.dropDst=word; m.Run(true); assert(!m.outputWrites && !m.mem[0x17ffc4/4]); }
	for (unsigned field=0;field<4;field++) { DivideProbeMachine m(image); m.mem[0x74000/4+field]^=1; m.Run(); assert(!m.outputWrites && !m.mmioWrites && !m.mem[0x17ffc4/4]); }
	for (unsigned word=0;word<64;word++) for (uint32_t bad:{0U,1U,0x007fffffU,0x7f800000U,0x7fc00000U,0xbf800000U}) {
		DivideProbeMachine m(image); m.mem[(word<32?0x90000:0x90800)/4+(word&31)]=bad;
		m.Run(); assert(!m.outputWrites && !m.mmioWrites && !m.mem[0x17ffc4/4]);
	}
	{ DivideProbeMachine m(image); m.dropAck=true; m.Run(true); assert(!m.outputWrites && !m.mem[0x17ffc4/4]); }
	printf("PASS OFFLINE actual RV32/SFPU exact divide,32768 IEEE outputs,ALL64 lost Dst writes/input-domain faults,descriptor/timeouts/guard words,unchanged strict ISA,no NoC/CSR/PCI/control;max steps%u stack%u;NOT native speed\n",maximumSteps,maximumStack);
}
