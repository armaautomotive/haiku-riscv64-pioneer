/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LOCAL_Q4_H
#define PIONEER_Q38_LOCAL_Q4_H
#include "PioneerQ38PredecodedQ4.h"
// OFFLINE DATA helper only,not an image/driver/source/layout selector.
// Expand ONE already-owned immutable block locally,using word operations
// instead of a byte-store loop. No model copy,learned products,IO,address
// authority,cache/session persistence or completion decisions live here.
// Caller independently owes aligned/disjoint saved L1 scratch,an immutable
// source and fences before hardware consumers. Little-endian words match
// the original packed GGUF tile and the prior bytewise format study.
struct PioneerQ38LocalQ4 {
	static constexpr unsigned SourceWords=PioneerQ38PredecodedQ4::OriginalBytes/4;
	static constexpr unsigned OutputWords=PioneerQ38PredecodedQ4::Bytes/4;
#ifdef PIONEER_ATTENTION_FREESTANDING
	__attribute__((noinline,section(".text.pioneer_q38_local_q4_expand")))
#endif
	static bool Expand(const volatile uint32_t* source,unsigned sourceWords,
		volatile uint32_t* output,unsigned outputWords)
	{
		uintptr_t a=reinterpret_cast<uintptr_t>(source),b=reinterpret_cast<uintptr_t>(output);
		if(!source || !output || (a&3) || (b&3) || sourceWords!=SourceWords || outputWords!=OutputWords
			|| !PioneerQ38PredecodedQ4::Separate(a,SourceWords*4,b,OutputWords*4))return false;
		for(unsigned word=0;word<64;word++)output[word]=source[word];
		for(unsigned pair=0;pair<8;pair++) {
			unsigned panel=(pair/2)*4+(pair%2);
			unsigned from=64+pair*64,low=64+panel*64,high=low+128;
			for(unsigned word=0;word<64;word++) {
				uint32_t packed=source[from+word];
				output[low+word]=packed&0x0f0f0f0fU;
				output[high+word]=(packed>>4)&0x0f0f0f0fU;
			}
		}
		return true;
	}
};
#endif
