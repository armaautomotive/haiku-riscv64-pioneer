/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#include "../PioneerQ38BFloatOperand.h"
struct BFloatEmitter {
	NativeF32Q8Machine& machine; unsigned count=0;
	void Op(uint32_t word) { if (word!=0x8f000000) machine.Instruction(word); count++; }
};
static int Raw(unsigned type,unsigned panel,unsigned low,unsigned high)
{
	if (type==8) return int(low)-int((low&128)*2);
	if (type==12) return int((low>>(((panel/2)%2)*4))&15);
	unsigned plane=(panel%8)/2;
	return int(((low>>((plane/2)*4))&15)|(((high>>(plane*2))&3)<<4))-32;
}
// Independent pinned MVMUL BF16 phases:SrcA implicit+high4 mantissa bits,
// then remainder;SrcB implicit+high6 bits already holds ALL signed8 values.
static float APhase(float a,unsigned phase)
{
	float high=Float(Bits(a)&0xfff80000U);
	return phase?a-high:high;
}
int main()
{
	(void)Read; (void)Word;
	NativeF32Q8Machine m(1024,32,0); unsigned operands=0,maximum=0,valid=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned panel=0;panel<(type==8?2U:16U);panel++) {
		unsigned combinations=type==14?65536:256;
		for (unsigned first=0;first<combinations;first+=32) {
			for (unsigned lane=0;lane<32;lane++) { m.lanes[0][lane]=(first+lane)&255; m.lanes[1][lane]=(first+lane)>>8; }
			BFloatEmitter e{m}; assert(PioneerQ38BFloatOperand(e,type,panel) && !m.conditional); maximum=std::max(maximum,e.count);
			for (unsigned lane=0;lane<32;lane++) {
				int raw=Raw(type,panel,(first+lane)&255,(first+lane)>>8);
				assert(m.lanes[0][lane]==Bits(float(raw)) && !(m.lanes[0][lane]&65535)); operands++;
			}
		}
	}
	for (unsigned input=0;input<65536;input++) {
		uint16_t out=0x7fc1; bool okay=PioneerQ38BFloatActivation(uint16_t(input),out);
		assert(okay==PioneerInt8OperandValid(uint16_t(input)));
		if (okay) { int value=int(input&1023)*((input&0x8000)?-1:1); assert(Float(uint32_t(out)<<16)==float(value)); valid++; }
		else assert(out==0x7fc1);
	}
	unsigned pairs=0;
	for (unsigned type:{8U,12U,14U}) {
		int first=type==12?0:type==14?-32:-128,last=type==12?15:type==14?31:127;
		for (int a=first;a<=last;a++) for (int b=-128;b<128;b++) {
			float bf=Float(Bits(float(b))&0xfffe0000U); assert(bf==float(b));
			float value=APhase(float(a),0)*bf;
			if (type==8) value+=APhase(float(a),1)*bf;
			assert(value==float(a*b)); pairs++;
		}
	}
	uint32_t seed=0xb51638a7; unsigned dots=0;
	auto random=[&]() { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; };
	for (unsigned type:{8U,12U,14U}) for (unsigned trial=0;trial<32768;trial++) {
		int a[16],b[16],dot=0;
		for (unsigned k=0;k<16;k++) {
			a[k]=type==8?int(random()&255)-128:type==12?int(random()&15):int(random()&63)-32;
			b[k]=int(random()&255)-128; dot+=a[k]*b[k];
		}
		float accumulated=0;
		for (unsigned phase=0;phase<(type==8?2U:1U);phase++) {
			float partial=0; for (unsigned k=0;k<16;k++) partial+=APhase(float(a[k]),phase)*float(b[k]);
			accumulated+=partial;
		}
		assert(accumulated==float(dot) && std::abs(dot)<=262144); dots++;
	}
	for (unsigned type:{0U,8U,12U,14U,99U}) { BFloatEmitter e{m}; assert(!PioneerQ38BFloatOperand(e,type,type==8?2:16) && !e.count); }
	printf("PASS OFFLINE BF16 integer prerequisite:%u exact raw operands/%u valid activation encodings incl canonical zero,-128 retained,%u exhaustive products,%u exact16-term dots;Q8 TWO/Q4+Q6 ONE mathematical fidelity phase,%u max vector instructions;NO image/format/driver/native/speed qualification\n",operands,valid,pairs,dots,maximum);
}
