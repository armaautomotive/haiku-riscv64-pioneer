/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
#include <cassert>
#include <climits>
int main() {
	for (int q=-128;q<=127;q++) {
		uint16_t value=PioneerInt8Operand(q);
		assert(PioneerInt8OperandValid(value));
		int decoded=int(value&1023)*((value&0x8000)?-1:1); assert(decoded==q);
	}
	assert(!PioneerInt8OperandValid(PioneerInt8Operand(-129)) && !PioneerInt8OperandValid(PioneerInt8Operand(128)));
	assert(!PioneerInt8OperandValid(PioneerInt8Operand(INT_MIN)) && !PioneerInt8OperandValid(PioneerInt8Operand(INT_MAX)));
	unsigned valid=0;
	for (unsigned value=0;value<65536;value++) {
		bool expected=(value&0x7c00)==0x4000 && (value&1023)<=((value&0x8000)?128U:127U);
		assert(PioneerInt8OperandValid(uint16_t(value))==expected); valid+=expected;
	}
	assert(valid==257); // includes both signs of zero
}
