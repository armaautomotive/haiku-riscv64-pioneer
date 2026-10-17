/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_RUNTIME_Q8_UNPACK_GATE_IMAGE_H
#define BLACKHOLE_RUNTIME_Q8_UNPACK_GATE_IMAGE_H
#include "BlackholeRuntimeQ8AccumImage.h"
// Explicit candidate: exactly ONE word differs from qualified exact math.
// SHA256 8180a243ebc308bf4942ff925da511ed58c8485e7a9b5ef043ca4c61c2a4e307
// All preceding/final drains, four fidelity phases, FMA and NoC bytes remain.
// Instruction/ownership/full-model/native gates are mandatory, not implied.
static inline uint32_t BlackholeRuntimeQ8UnpackGateCodeWord(unsigned word)
{
	return word==0x370/4?0x00000013:BlackholeRuntimeQ8AccumCodeWord(word);
}
#endif
