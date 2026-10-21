/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#include "../PioneerQ38NativeOperand.h"
#include "../PioneerQ38SignedMagnitude2.h"
// Separate independent extension. Do NOT loosen the qualified interpreter's
// opcode/modifier whitelist, or give any compiled image a new permission.
struct SignedMagnitudeMachine:NativeF32Q8Machine {
	using NativeF32Q8Machine::NativeF32Q8Machine;
	void Instruction(uint32_t op)override {
		unsigned code=op>>24,vc=(op>>8)&15,vd=(op>>4)&15;
		if(code==0x7d && !(op&15)) {
			assert(!(op&0x00fff000)&&vc<8&&vd<8);
			for(unsigned lane=0;lane<32;lane++)if(!conditional||flags[lane]) {
				uint32_t x=lanes[vc][lane];lanes[vd][lane]=(x&0x80000000U)?0U-x:x;
			}
			drained=false;
			return;
		}
		if(code==0x89) {
			assert(!(op&0x00fff00f)&&vc<8&&vd<8);
			for(unsigned lane=0;lane<32;lane++)if(!conditional||flags[lane]) {
				uint32_t magnitude=lanes[vc][lane]&0x7fffffffU,sign=lanes[vd][lane]&0x80000000U;
				lanes[vd][lane]=sign|magnitude;
			}
			drained=false;
			return;
		}
		NativeF32Q8Machine::Instruction(op);
	}
};
template<class M>struct SignedEmitter {M& m;unsigned count=0;void Op(uint32_t op){m.Instruction(op);count++;}};
int main()
{
	(void)Read;(void)Word;unsigned cases=0;
	for(unsigned mask:{0U,0x55555555U,0xaaaaaaaaU,0xffffffffU})for(unsigned first=0;first<65536;first+=32) {
		SignedMagnitudeMachine next(1024,32,0);NativeF32Q8Machine old(1024,32,0);
		for(unsigned lane=0;lane<32;lane++) {
			for(unsigned reg=0;reg<8;reg++)next.lanes[reg][lane]=old.lanes[reg][lane]=0xdace0000+reg*32+lane;
			next.lanes[0][lane]=old.lanes[0][lane]=uint32_t(int(first+lane)-32768);
			next.flags[lane]=old.flags[lane]=(mask>>lane)&1;
		}
		next.conditional=old.conditional=true;
		SignedEmitter<SignedMagnitudeMachine> a{next};SignedEmitter<NativeF32Q8Machine> b{old};
		PioneerQ38SignedMagnitude2(a);PioneerQ38NativeSignMagnitude(b);assert(a.count==2&&b.count==8&&next.drained==old.drained);
		for(unsigned lane=0;lane<32;lane++) {
			int64_t v=int(first+lane)-32768;uint32_t expected=(mask>>lane)&1?
				(v<0?0x80000000U:0U)|uint32_t(v<0?-v:v):uint32_t(v);
			assert(next.lanes[0][lane]==expected&&next.lanes[0][lane]==old.lanes[0][lane]);
			assert(next.conditional&&next.flags[lane]==bool((mask>>lane)&1));
			for(unsigned reg:{1U,3U,4U,5U,6U,7U})assert(next.lanes[reg][lane]==0xdace0000+reg*32+lane);
			cases++;
		}
	}
	SignedMagnitudeMachine edge(1024,32,0);unsigned i=0;
	for(uint32_t value:{0U,1U,127U,128U,0x7fffffffU,0xffffffffU,0xffffff80U,0x80000001U})edge.lanes[0][i++]=value;
	SignedEmitter<SignedMagnitudeMachine> e{edge};PioneerQ38SignedMagnitude2(e);
	const uint32_t expected[]={0,1,127,128,0x7fffffff,0x80000001,0x80000080,0xffffffff};
	for(unsigned lane=0;lane<8;lane++)assert(edge.lanes[0][lane]==expected[lane]);
	for(unsigned wrong=0;wrong<3;wrong++) {
		SignedMagnitudeMachine bad(1024,32,0);bad.lanes[0][0]=0xffffff80U;bad.lanes[2][0]=0x12345678;
		// Wrong float ABS, reversed SETSGN operands, or destroying DEST0's
		// original sign MUST NOT be mistaken for the qualified composition.
		bad.Instruction(wrong==0?0x7d000021:wrong==2?0x7d000000:0x7d000020);
		bad.Instruction(wrong==1?0x89000020:0x89000200);
		assert(bad.lanes[0][0]!=0x80000080U);
	}
	printf("PASS OFFLINE signed magnitude:%u exhaustive signed16/32-lane/enable-mask cases+8signed32edges;exact original8ops vs2ops,original DEST sign retained,other live regs/flags untouched;NO image/driver/native/throughput qualification\n",cases);
}
