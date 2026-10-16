/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q8_COMPACT_LAYOUT_H
#define PIONEER_Q8_COMPACT_LAYOUT_H
#include "BlackholeRuntimeMatvecImage.h"
// One 576-byte tile holds sixteen FP16 scales, 32x16 signed bytes and
// 32 padding bytes. It replaces TWO expanded integer-operand tiles.
// N multiples of 32 make every supported K's whole tensor/chunk page-aligned.
static inline bool PioneerQ8CompactTensorShape(unsigned inputs,unsigned outputs)
{
	return PioneerMatvecShape(inputs,32) && outputs>=32 && outputs<=262144 && !(outputs&31);
}
static inline bool PioneerQ8CompactShape(unsigned inputs,unsigned outputs)
{
	return outputs<=TT_RUNTIME_MATVEC_OUTPUTS_MAX && PioneerQ8CompactTensorShape(inputs,outputs);
}
static inline uint64_t PioneerQ8CompactWeightBytes(unsigned inputs,unsigned outputs)
{
	return PioneerQ8CompactTensorShape(inputs,outputs)?uint64_t(inputs/32)*(outputs/16)*576:0;
}
static inline bool PioneerQ8CompactWeightRange(uint64_t capacity,uint64_t offset,unsigned inputs,unsigned outputs)
{
	uint64_t bytes=PioneerQ8CompactWeightBytes(inputs,outputs);
	return PioneerQ8CompactShape(inputs,outputs) && bytes && PioneerResidentOffset(capacity,offset) && bytes<=capacity-offset;
}
static inline uint64_t PioneerQ8CompactScaleByte(unsigned inputs,unsigned block,unsigned output)
{
	return (uint64_t(output/16)*(inputs/32)+block)*576+(output%16)*2;
}
static inline uint64_t PioneerQ8CompactValueByte(unsigned inputs,unsigned k,unsigned output)
{
	return (uint64_t(output/16)*(inputs/32)+k/32)*576+32+(k%32)*16+(output%16);
}
#endif
