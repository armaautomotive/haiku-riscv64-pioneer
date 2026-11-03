/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_native_bfp_machine.h"
#include "../PioneerQ38NativeFidelityView.h"
#include <cstdio>
int main() {
	unsigned cases=0;
	for(unsigned raw=0;raw<=65535;raw++) {
		const unsigned exponent=(raw>>7)&255;if(exponent==255 || (!exponent && (raw&127)))continue;
		const float input=NativeBfpMachine::Float(raw<<16);
		for(unsigned significant:{5U,7U}) {
			float got=19;assert(PioneerQ38NativePhase0View(input,significant,got));
			const float independent=NativeBfpMachine::Part(input,0,significant==5);
			assert(NativeBfpMachine::Bits(got)==NativeBfpMachine::Bits(independent));cases++;
		}
	}
	for(uint32_t invalid:{1U,0x80000001U,0x7f800000U,0xff800000U,0x7fc00001U})for(unsigned significant:{5U,7U}) {
		float got=19;assert(!PioneerQ38NativePhase0View(NativeBfpMachine::Float(invalid),significant,got) && got==19);cases++;
	}
	for(unsigned invalid:{0U,1U,4U,6U,8U,24U,~0U}){float got=19;assert(!PioneerQ38NativePhase0View(1,invalid,got) && got==19);cases++;}
	printf("PASS CPU-only native phase0 operand VIEW:%u cases;all finite-normal/zero BF16 patterns agree with separate frexp/ldexp arithmetic for5/7 significant bits,invalid/nonfinite/subnormal refusal atomic;NOT full FPU/model/hardware/quality/speed proof\n",cases);
}
