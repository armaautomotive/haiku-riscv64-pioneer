/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#include "../PioneerQ38NativeOperand.h"
#include "../PioneerQ38Q4Nibble.h"
struct NibbleEmitter {
	NativeF32Q8Machine& m;unsigned count=0;
	void Op(uint32_t op){m.Instruction(op);count++;}
};
int main()
{
	(void)Read;(void)Word;unsigned cases=0;
	for(unsigned panel=0;panel<16;panel++)for(unsigned job=0;job<3;job++) {
		NativeF32Q8Machine next(1024,32,0),old(1024,32,0);
		for(unsigned reg=0;reg<8;reg++)for(unsigned lane=0;lane<32;lane++)
			next.lanes[reg][lane]=old.lanes[reg][lane]=0xdac00000U+job*4096+panel*256+reg*32+lane;
		NibbleEmitter setup{next};assert(PioneerQ38Q4NibbleSetup(setup,panel));
		assert(setup.count==unsigned(!(panel&2)));
		unsigned proposed=setup.count,previous=0;
		for(unsigned chunk=0;chunk<8;chunk++) {
			for(unsigned lane=0;lane<32;lane++)next.lanes[0][lane]=old.lanes[0][lane]=(chunk*32+lane+job*79)&255;
			NibbleEmitter a{next},b{old};
			assert(PioneerQ38Q4NibbleDecode(a,panel)&&PioneerQ38NativeOperand(b,12,panel));
			assert(a.count==1&&b.count==PioneerQ38NativeOperandCount(12,panel));
			proposed+=a.count;previous+=b.count;
			for(unsigned lane=0;lane<32;lane++) {
				unsigned raw=(chunk*32+lane+job*79)&255,value=(raw>>((panel&2)?4:0))&15;
				assert(next.lanes[0][lane]==value&&next.lanes[0][lane]==old.lanes[0][lane]);
				for(unsigned reg:{1U,2U,3U,5U,6U,7U})assert(next.lanes[reg][lane]==0xdac00000U+job*4096+panel*256+reg*32+lane);
				if(!(panel&2))assert(next.lanes[4][lane]==15);
				cases++;
			}
		}
		assert(proposed==((panel&2)?8U:9U)&&previous==((panel&2)?24U:16U));
	}
	NativeF32Q8Machine invalid(1024,32,0);NibbleEmitter e{invalid};
	for(unsigned panel:{16U,17U,~0U})assert(!PioneerQ38Q4NibbleSetup(e,panel)&&!PioneerQ38Q4NibbleDecode(e,panel)&&!e.count);
	printf("PASS OFFLINE Q4 nibble:%u exhaustive byte/panel/lane/changed-input cases;poisoned mask refreshed PER panel;low9vs16/high8vs24 expanded register ops incl setup;invalid selectors zero emission;NO native/speed claim\n",cases);
}
