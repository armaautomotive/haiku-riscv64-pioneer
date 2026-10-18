/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#include "../PioneerQ38VectorOperand.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
struct Emitter {
	NativeF32Q8Machine& machine; unsigned count=0;
	void Op(uint32_t word) { machine.Instruction(word); count++; }
};
static int Reference(unsigned type,unsigned panel,unsigned low,unsigned high)
{
	if (type==8) return int(low)-int((low&128)*2);
	if (type==12) return int((low>>(((panel/2)%2)*4))&15);
	unsigned plane=(panel%8)/2;
	return int(((low>>((plane/2)*4))&15)|(((high>>(plane*2))&3)<<4))-32;
}
int main()
{
	(void)Read; (void)Word;
	NativeF32Q8Machine machine(1024,32,0); unsigned tested=0,maximum=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned panel=0;panel<(type==8?2U:16U);panel++) {
		unsigned combinations=type==14?65536:256;
		for (unsigned first=0;first<combinations;first+=32) {
			for (unsigned lane=0;lane<32;lane++) {
				machine.lanes[0][lane]=(first+lane)&255; machine.lanes[1][lane]=(first+lane)>>8;
			}
			Emitter e{machine}; assert(PioneerQ38VectorOperand(e,type,panel) && !machine.conditional); maximum=std::max(maximum,e.count);
			for (unsigned lane=0;lane<32;lane++) {
				int value=Reference(type,panel,(first+lane)&255,(first+lane)>>8);
				uint32_t actual=machine.lanes[0][lane]; unsigned magnitude=unsigned(value<0?-value:value);
				uint32_t expected=(value<0?0x80000000U:0)|(magnitude<<21)|0x00100000;
				assert(actual==expected && !(actual&65535));
				uint16_t high=uint16_t(actual>>16);
				uint32_t physicalSource=((uint32_t(high)&0xffe0)<<3)|(high&31); // documented MOVD2A ShuffleFP16
				uint16_t source=uint16_t(((physicalSource&0x40000)>>3)|((physicalSource&31)<<10)|((physicalSource>>8)&1023));
				assert(source==PioneerInt8Operand(value) && PioneerInt8OperandValid(source)); tested++;
			}
		}
	}
	for (unsigned type:{0U,8U,12U,14U,99U}) {
		Emitter e{machine}; assert(!PioneerQ38VectorOperand(e,type,type==8?2:16) && !e.count);
	}
	printf("PASS OFFLINE vector operand emitter:%u exhaustive Q8/Q4/Q6 raw-byte/plane cases,ALL32 lanes,exact signed -128 and Dst32-high16->SrcA overlays,%u maximum existing integer SFPU instructions per32 weights,no conditional flags/FP approximations;NO unpack/Dst scheduling/image/driver/native/speed qualification\n",tested,maximum);
}
