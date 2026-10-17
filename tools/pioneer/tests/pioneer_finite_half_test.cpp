/* SPDX-License-Identifier: MIT */
#include "../PioneerFiniteHalf.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
	unsigned cases=0;
	for (unsigned bits=0;bits<65536;bits++) {
		if ((bits&0x7c00)==0x7c00) continue; // UNCHANGED entry refuses ALL these encodings
		uint16_t value=uint16_t(bits); _Float16 half; memcpy(&half,&value,2); float independent=float(half);
		assert(PioneerFiniteHalfBits(value)==PioneerAttentionBits(independent));
		assert(PioneerFiniteHalfBits(value)==PioneerAttentionBits(PioneerAttentionHalf(value))); cases++;
	}
	assert(cases==63488);
	puts("PASS OFFLINE exhaustive63488 finite-half encodings,independent IEEE widening/all sign/zero/subnormal bits;NOT input validation/native performance");
}
