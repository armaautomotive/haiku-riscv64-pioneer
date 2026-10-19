/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ACTIVATION_PACKING_H
#define PIONEER_Q38_ACTIVATION_PACKING_H
#include "PioneerInt8Operand.h"
// Lossless transport ONLY. Preserve signed zero as well as -128.
// Three 9-bit sign/magnitude values per 32-bit word;upper five bits zero.
// Arithmetic still consumes the EXACT original 16-bit source overlay.
static inline unsigned PioneerQ38PackedActivationWords(unsigned inputs)
{ return inputs==5120?1707:inputs==6144?2048:inputs==17408?5803:0; }
static inline uint16_t PioneerQ38ExpandActivation(unsigned bits)
{ return uint16_t(0x4000|(bits&255)|((bits&256)<<7)); }
static inline bool PioneerQ38PackActivationWord(const uint32_t* input,unsigned inputs,unsigned word,uint32_t& packed)
{
	if (!input || (inputs!=5120 && inputs!=6144 && inputs!=17408) || word>=PioneerQ38PackedActivationWords(inputs)) return false;
	uint32_t value=0;
	for (unsigned lane=0;lane<3;lane++) {
		unsigned index=word*3+lane; if (index>=inputs) break;
		uint16_t source=uint16_t(input[index/2]>>((index&1)*16));
		if (!PioneerInt8OperandValid(source)) return false;
		value|=((source&255)|((unsigned(source)>>7)&256))<<(lane*9);
	}
	packed=value; return true;
}
#endif
