/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_RUNTIME_FP16_MATVEC_IMAGE_H
#define BLACKHOLE_RUNTIME_FP16_MATVEC_IMAGE_H
#include "BlackholeRuntimeMatvecImage.h"
// Independently assembled TT_RUNTIME_FP16 variant. The only differences
// are the six fixed source-format immediates, not code/data addresses,
// NoC commands, loops, phase/park markers or FP32 destination precision.
static inline uint32_t BlackholeRuntimeFp16MatvecCodeWord(unsigned word)
{
	switch (word) {
		case 37: return 0x23128293; // config 0: SrcA/SrcB format 1 overrides
		case 41: return 0x202202b7; // config 1: format 1 fields, FP32 accumulate
		case 91: case 123: return 0x01128293; // unpack descriptors format 1
		case 99: case 131: return 0x00100293; // unpack output format 1
		default: return BlackholeRuntimeMatvecCodeWord(word);
	}
}
#endif
