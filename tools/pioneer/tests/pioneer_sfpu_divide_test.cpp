/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#include "../PioneerSfpuDivide.h"
struct DivideEmitter {
	NativeF32Q8Machine& machine; unsigned words=0;
	void Op(uint32_t word) { machine.Instruction(word); words++; }
};
int main()
{
	(void)Read; (void)Word;
	NativeF32Q8Machine machine(1024,32,0); uint32_t seed=0x3651ab9d;
	auto next=[&]() { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; };
	unsigned count=0,wordCount=0;
	for (unsigned trial=0;trial<40960;trial++) {
		uint32_t expected[32];
		for (unsigned lane=0;lane<32;lane++) {
			uint32_t a,b;
			do { a=next()&0x7fffffff; b=next()&0x7fffffff; } while (!PioneerSfpuDivideDomain(a,b));
			if (trial<253) { a=(trial+1)<<23; b=0x3f800000; }
			if (trial>=253 && trial<506) { a=((trial-252)<<23)|0x7fffff; b=0x3f800000; }
			if (trial>=506 && trial<759) { a=((trial-505)<<23)|(lane&1?0x7fffff:0); b=lane&1?0x3f800001:0x3fffffff;
				if (!PioneerSfpuDivideDomain(a,b)) { a=0x3f800000; b=0x3f800000; } }
			assert(PioneerSfpuDivideDomain(a,b));
			machine.lanes[0][lane]=a; machine.lanes[1][lane]=b;
			expected[lane]=Bits(Float(a)/Float(b)); count++;
		}
		DivideEmitter emitter={machine}; PioneerSfpuDivideNormal(emitter);
		if (!trial) wordCount=emitter.words; assert(emitter.words==wordCount && !machine.conditional);
		for (unsigned lane=0;lane<32;lane++) {
			if (machine.lanes[0][lane]!=expected[lane]) { printf("DIV TRACE trial%u lane%u actual%08x expected%08x\n",trial,lane,machine.lanes[0][lane],expected[lane]); fflush(stdout); }
			assert(machine.lanes[0][lane]==expected[lane]);
		}
	}
	for (uint32_t bad:{0U,1U,0x007fffffU,0x7f800000U,0x7fc00000U,0x80000000U,0xbf800000U,UINT32_MAX})
		assert(!PioneerSfpuDivideDomain(bad,0x3f800000) && !PioneerSfpuDivideDomain(0x3f800000,bad));
	assert(!PioneerSfpuDivideDomain(0x00800000,0x7f000000) && !PioneerSfpuDivideDomain(0x7f000000,0x00800000));
	printf("PASS OFFLINE exact32-lane integer SFPU division,%u independent IEEE quotients,all normal exponent edges,unchanged strict SFPU instruction/modifier model,%u trusted instructions/batch,NO NoC/CSR/FP approximations/native claim\n",count,wordCount);
	return 0;
}
