/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_INT8_OPERAND_H
#define PIONEER_INT8_OPERAND_H
#ifdef PIONEER_ATTENTION_FREESTANDING
typedef __UINT16_TYPE__ uint16_t;
#else
#include <stdint.h>
#endif
// Integer "8" source values overlay FP16: sign, fixed raw exponent 16,
// magnitude. This is data packing, NOT floating-point multiplication.
static inline bool PioneerInt8OperandValid(uint16_t value)
{
	unsigned magnitude=value&1023;
	return (value&0x7c00)==0x4000 && magnitude<=((value&0x8000)?128U:127U);
}
static inline uint16_t PioneerInt8Operand(int value)
{
	if (value<-128 || value>127) return 0; // refused by the validity predicate
	return uint16_t(0x4000|(value<0?0x8000|unsigned(-value):unsigned(value)));
}
#endif
