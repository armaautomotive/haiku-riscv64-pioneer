/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#include "../PioneerQ38NativeRawOperand.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
// Separate exact decode of the ONE new register-shift form. The existing
// strict RV32 Step and existing SFPU observer are deliberately unchanged.
struct RawMachine:NativeF32Q8Machine {
	unsigned registerShifts=0;bool dropRegisterShift=false;
	RawMachine():NativeF32Q8Machine(1024,32,0){}
	void Instruction(uint32_t word) override {
		if((word>>24)==0x77 || (word>>24)==0x78) {
			assert(!conditional && (word==0x77000010 || word==0x78000030));
			for(unsigned lane=0;lane<32;lane++) {
				const uint32_t input=lanes[0][lane];
				if(word==0x77000010)lanes[1][lane]=uint32_t(int((input>>23)&255)-127);
				else lanes[3][lane]=0x800000|(input&0x7fffff);
			}
			return;
		}
		if((word>>24)==0x7a && !(word&15)) {
			assert((word==0x7a000100 || word==0x7a000130) && !conditional);registerShifts++;
			const unsigned target=(word>>4)&15;
			for(unsigned lane=0;lane<32;lane++) {
				const int32_t amount=int32_t(lanes[1][lane]);
				assert((amount>=-30 && amount<=-17) || amount==-150 || amount==-148 || amount==-144 || amount==105 || amount==107 || amount==111);
				if(!dropRegisterShift) {
					if(amount>=0)lanes[target][lane]<<=unsigned(amount)&31;
					else lanes[target][lane]>>=unsigned(-amount)&31;
				}
			}
			return;
		}
		NativeF32Q8Machine::Instruction(word);
	}
};
struct RawEmitter {
	RawMachine& machine;unsigned count=0;
	void Op(uint32_t word){machine.Instruction(word);count++;}
};
static uint32_t UnpackBits(unsigned bits,unsigned code) {
	assert((bits==2 || bits==4 || bits==8) && code<(1U<<bits));
	const unsigned sign=code>>(bits-1),magnitude=code&((1U<<(bits-1))-1);
	if(!magnitude)return sign?0xff800000U:0;
	unsigned highest=0;for(unsigned n=magnitude;n>>=1;)highest++;
	const unsigned exponent=127-(bits-2)+highest;
	return (sign<<31)|(exponent<<23)|((magnitude<<(23-highest))&0x7fffff);
}
static void CheckOverlay(uint32_t actual,int expected) {
	const unsigned magnitude=unsigned(expected<0?-expected:expected);
	assert(actual==((expected<0?0x80000000U:0)|(magnitude<<21)|0x00100000));
	const uint16_t high=uint16_t(actual>>16);
	const uint32_t physical=((uint32_t(high)&0xffe0)<<3)|(high&31);
	const uint16_t overlay=uint16_t(((physical&0x40000)>>3)|((physical&31)<<10)|((physical>>8)&1023));
	assert(overlay==PioneerInt8Operand(expected) && PioneerInt8OperandValid(overlay));
}
#ifndef PIONEER_NATIVE_RAW_MACHINE_ONLY
int main() {
	(void)Read;(void)Word;RawMachine machine,baseline;unsigned recovered=0,overlays=0,maximum=0,baselineMaximum=0;
	for(unsigned bits:{2U,4U,8U})for(unsigned first=0;first<(1U<<bits);first++) {
		for(unsigned lane=0;lane<32;lane++) {
			machine.lanes[0][lane]=UnpackBits(bits,(first+lane)% (1U<<bits));
			baseline.lanes[0][lane]=machine.lanes[0][lane];machine.flags[lane]=baseline.flags[lane]=bool(lane&1);
			for(unsigned reg=4;reg<=6;reg++)machine.lanes[reg][lane]=baseline.lanes[reg][lane]=0x5a000000U|(reg<<16)|lane;
		}
		RawEmitter emit{machine},control{baseline};const unsigned shifts=machine.registerShifts;
		assert(PioneerQ38NativeRawRecover(emit,bits) && !machine.conditional && machine.registerShifts==shifts+1);
		assert(PioneerQ38NativeRawRecoverBaseline(control,bits) && !baseline.conditional);
		maximum=std::max(maximum,emit.count);baselineMaximum=std::max(baselineMaximum,control.count);
		for(unsigned lane=0;lane<32;lane++) {
			assert(machine.lanes[0][lane]==(first+lane)% (1U<<bits));
			assert(machine.lanes[0][lane]==baseline.lanes[0][lane] && machine.flags[lane]==bool(lane&1) && baseline.flags[lane]==bool(lane&1));
			for(unsigned reg=4;reg<=6;reg++)assert(machine.lanes[reg][lane]==(0x5a000000U|(reg<<16)|lane) && baseline.lanes[reg][lane]==machine.lanes[reg][lane]);
			recovered++;
		}
		if(bits==4 || bits==8) {
			assert(PioneerQ38VectorOperand(emit,bits==4?12:8,0));
			for(unsigned lane=0;lane<32;lane++) {
				int code=int((first+lane)% (1U<<bits));if(bits==8 && code>=128)code-=256;
				CheckOverlay(machine.lanes[0][lane],code);overlays++;
			}
		}
	}
	// Q6 is raw low4 + raw high2, with its original signed bias of32.
	// A future caller must supply the two independent native-unpack loads.
	for(unsigned first=0;first<64;first++) {
		for(unsigned lane=0;lane<32;lane++)machine.lanes[0][lane]=UnpackBits(2,((first+lane)%64)>>4);
		RawEmitter emit{machine};assert(PioneerQ38NativeRawRecover(emit,2));PioneerSfpuMove(emit,6,0);
		for(unsigned lane=0;lane<32;lane++)machine.lanes[0][lane]=UnpackBits(4,(first+lane)%16);
		assert(PioneerQ38NativeRawRecover(emit,4));PioneerSfpuShift(emit,6,4);PioneerSfpuAdd(emit,0,6);
		PioneerSfpuAddI(emit,0,-32);PioneerQ38VectorSignedOverlay(emit);
		for(unsigned lane=0;lane<32;lane++){CheckOverlay(machine.lanes[0][lane],int((first+lane)%64)-32);overlays++;}
	}
	for(unsigned invalid:{0U,1U,3U,5U,6U,16U}){RawEmitter emit{machine};assert(!PioneerQ38NativeRawRecover(emit,invalid) && !emit.count);}
	// Negative controls: dropping the special sign code or the new shift
	// cannot accidentally pass as a correct lossless recovery.
	for(unsigned lane=0;lane<32;lane++)machine.lanes[0][lane]=0; // raw8 incorrectly collapsed to zero
	{RawEmitter emit{machine};assert(PioneerQ38NativeRawRecover(emit,4));for(unsigned lane=0;lane<32;lane++)assert(machine.lanes[0][lane]!=8);}
	for(unsigned lane=0;lane<32;lane++)machine.lanes[0][lane]=UnpackBits(4,6);
	machine.dropRegisterShift=true;
	{RawEmitter emit{machine};assert(PioneerQ38NativeRawRecover(emit,4));for(unsigned lane=0;lane<32;lane++)assert(machine.lanes[0][lane]!=6);}
	assert(maximum==9 && baselineMaximum==22);
	printf("PASS OFFBOARD actual native-raw SFPU recovery:%u all-domain/lane raw codes,%u exact Q4/Q6/Q8 overlay cases,negative-infinity raw2/8/128 and Q8 -128 preserved,%u recovery instructions/32 weights versus%u baseline,flags/preserved registers unchanged;NO scalar dequant/FP approximation;NO actual unpack/MOVA2D/scheduling/image/driver/hardware/quality/speed qualification\n",recovered,overlays,maximum,baselineMaximum);
}
#endif
