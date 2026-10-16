/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_RUNTIME_Q8_READBACK_IMAGE_H
#define BLACKHOLE_RUNTIME_Q8_READBACK_IMAGE_H
#include "BlackholeRuntimeQ8BulkImage.h"
// OFFLINE-ONLY candidate. No driver opcode/capability or native selector
// exposes this image. Math bytes/park remain identical; only the alternate
// entry and previously unused164-byte tail implement exact readback compare.
enum {
	TT_Q8_READBACK_ENTRY=0x7556a06f,TT_Q8_READBACK_DISPATCH_PC=0x70f54,
	TT_Q8_READBACK_PHASE=0x70fb4,TT_Q8_READBACK_PARK_PC=0x70fe8,
	TT_Q8_READBACK_END=0x70ff8,TT_Q8_READBACK_MARKER=0x52565731,
	TT_Q8_READBACK_MAX_WORDS=2048
};
// Metadata subpredicate ONLY, not authority to release/accept a core. A future
// controller must independently prove a freshly cleared/read-back FOUR-word
// status record, immutable image/descriptor, mappings/owner/source/PCI/NoC
// stability, BME off before launch, zero new NoC traffic and verified stop.
static inline bool BlackholeRuntimeReadbackProof(uint32_t phase,uint32_t words,uint32_t marker,
	uint32_t mismatchMask,uint32_t pc0,uint32_t pc1,unsigned expected)
{
	return expected>=16 && expected<=TT_Q8_READBACK_MAX_WORDS && !(expected&15)
		&& phase==TT_Q8_READBACK_PHASE && words==expected && marker==TT_Q8_READBACK_MARKER && !mismatchMask
		&& pc0==TT_Q8_READBACK_PARK_PC && pc1==pc0;
}
static inline uint32_t BlackholeRuntimeQ8ReadbackCodeWord(unsigned word)
{
	if (word<0xf54/4) return BlackholeRuntimeQ8BulkCodeWord(word);
	static const uint32_t tail[]={
		0x00070537,0x50050513,0x00c52283,0x00100313,0x00628663,0x88028c63,0x0800006f,0x00852583,
		0xfff58293,0x0072d293,0x06029863,0x00459593,0x00058893,0x00084637,0x000886b7,0x00000713,
		0x00062283,0x0006a303,0x0062c2b3,0x00576733,0x00460613,0x00468693,0xfff58593,0xfe0592e3,
		0x00180537,0xfc050513,0x01152223,0x00e52623,0x00000297,0xff028293,0x00552023,0x525652b7,
		0x73128293,0x00552423,0x00852283,0x0002f293,0x0ff0000f,0x0000006f,0x00000893,0xfff00713,
		0xfc1ff06f,0x00000000,0x00000000
	};
	return word<1024?tail[word-0xf54/4]:0;
}
#endif
