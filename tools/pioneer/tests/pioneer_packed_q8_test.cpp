/* SPDX-License-Identifier: MIT */
#include <initializer_list>
#define PIONEER_GRAPH_PACKED_Q8 1
#define PIONEER_GRAPH_MATH_UNIT_ONLY
#define main PioneerGraphOriginalTests
#include "pioneer_graph_math_test.cpp"
#undef main
static void Check(uint32_t bits)
{
	float value=PioneerAttentionFloat(bits); int original=PioneerGraphRoundQ8(value);
	bool valid=original>=-128 && original<=127; uint32_t encoded=0xdeadbeef;
	assert(PioneerGraphPackQ8Operand(value,encoded)==valid);
	if (valid) assert(encoded==PioneerInt8Operand(original));
	else assert(encoded==0xdeadbeef);
}
int main()
{
	// Every binary32 value from.5 through128,including ALL half-way ties
	// and adjacent floats,for BOTH signs;no float-to-int oracle shortcut.
	for (uint32_t bits=0x3f000000;bits<=0x43000000;bits++) { Check(bits); Check(bits|0x80000000); }
	for (uint32_t bits:{0U,1U,0x007fffffU,0x00800000U,0x3effffffU,0x43000001U,0x7f7fffffU,0x7f800000U,0x7fc00000U,0x7fffffffU}) { Check(bits); Check(bits|0x80000000); }
	for (unsigned trial=0;trial<1000000;trial++) Check(Next());
	assert(!PioneerGraphOriginalTests());
	puts("PASS OFFLINE exact direct Q8 packing,ALL134217730 signed boundary encodings,one million full-domain samples,canonical zero/negative128/refusal,20000 independent pinned quantizer blocks;NO native claim");
}
