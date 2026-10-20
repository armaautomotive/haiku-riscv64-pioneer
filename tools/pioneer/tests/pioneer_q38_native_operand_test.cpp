/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#include "../PioneerQ38NativeOperand.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
struct NativeOperandEmitter {
	NativeF32Q8Machine& machine; unsigned count=0;
	void Op(uint32_t word) { machine.Instruction(word); count++; }
};
struct StoreWord { unsigned count=0; uint32_t word=0; void Op(uint32_t v) { word=v; count++; } };
int main()
{
	(void)Read; (void)Word;
	NativeF32Q8Machine m(1024,32,0); unsigned cases=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned p=0;p<(type==8?2U:16U);p++) {
		unsigned combinations=type==14?65536:256;
		for (unsigned first=0;first<combinations;first+=32) {
			for (unsigned lane=0;lane<32;lane++) { m.lanes[0][lane]=(first+lane)&255; m.lanes[1][lane]=(first+lane)>>8; }
			NativeOperandEmitter next{m}; assert(PioneerQ38NativeOperand(next,type,p) && !m.conditional);
			assert(next.count==PioneerQ38NativeOperandCount(type,p));
			for (unsigned lane=0;lane<32;lane++) {
				unsigned low=(first+lane)&255,high=(first+lane)>>8,plane=(p%8)/2;
				int value=type==8?int(low)-int((low&128)*2):type==12?int((low>>(((p/2)%2)*4))&15):
					int(((low>>((plane/2)*4))&15)|(((high>>(plane*2))&3)<<4))-32;
				uint32_t sm=(value<0?0x80000000U:0)|unsigned(value<0?-value:value);
				assert(m.lanes[0][lane]==sm);
				// Independent mode5 FP16 physical encoding,then UNCHANGED
				// 32-bit MOVD2A high-half shuffle. No implementation helper.
				uint16_t stored=uint16_t(((sm>>16)&0x8000)|((sm&1023)<<5)|16);
				uint32_t source=((uint32_t(stored)&0xffe0)<<3)|(stored&31);
				uint16_t overlay=uint16_t(((source&0x40000)>>3)|((source&31)<<10)|((source>>8)&1023));
				assert(overlay==PioneerInt8Operand(value)); cases++;
			}
		}
	}
	for (unsigned block:{0U,4U,8U,12U}) for (unsigned parity:{0U,2U}) {
		unsigned logical=64+block,physical=((logical&0x1f8)<<1)|(logical&0x207);
		StoreWord e; assert(PioneerQ38NativeOperandRow(block)==physical && PioneerQ38NativeOperandStore(e,block,parity));
		assert(e.count==1 && e.word==(0x72050000|physical|parity));
		for (unsigned lane=0;lane<32;lane++) {
			unsigned logicalRow=logical+lane/8,hi=((logicalRow&0x1f8)<<1)|(logicalRow&0x207);
			assert(physical+lane/8==hi && hi>=128 && hi<160 && hi+8<160);
		}
	}
	for (unsigned block:{1U,2U,3U,15U,16U,~0U}) { StoreWord e; assert(!PioneerQ38NativeOperandStore(e,block,0) && !e.count); }
	for (unsigned parity:{1U,3U,4U,~0U}) { StoreWord e; assert(!PioneerQ38NativeOperandStore(e,0,parity) && !e.count); }
	for (unsigned type:{0U,8U,12U,14U,99U}) { NativeOperandEmitter e{m}; assert(!PioneerQ38NativeOperand(e,type,type==8?2:16) && !e.count); }
	printf("PASS OFFLINE native operand:%u exhaustive raw-byte/plane/all32-lane cases,exact Q8 -128/Q4/Q6 SM and original MOVD2A overlays,only existing owned initialized physical HIGH halves,invalid selectors zero emission;NO image/scheduling/driver/native/performance qualification\n",cases);
}
