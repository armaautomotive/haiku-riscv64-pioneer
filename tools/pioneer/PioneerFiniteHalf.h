/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_FINITE_HALF_H
#define PIONEER_FINITE_HALF_H
#include "PioneerAttentionMath.h"
// OFFLINE hot-loop specialization. PRECONDITION:the complete frozen K/V
// prefix was validated finite BEFORE any SFPU mutation,as in attention_main.
// No validation is removed from the entry. Normal finite halves use the
// direct IEEE exponent rebias;zero/subnormal signs and bits stay exact.
// Not a converter for unvalidated Inf/NaN and not a new supplied-data ABI.
static inline uint32_t PioneerFiniteHalfBits(uint16_t value)
{
	uint32_t magnitude=uint32_t(value)&0x7fff;
	if (magnitude>=0x400) return ((uint32_t(value)&0x8000)<<16)|((magnitude<<13)+0x38000000);
	return PioneerAttentionBits(PioneerAttentionHalf(value));
}
#endif
