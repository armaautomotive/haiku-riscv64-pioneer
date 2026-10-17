/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DECODER_GREEDY_H
#define PIONEER_DECODER_GREEDY_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// Distinct fixed128B result,not a vocabulary array or a checksum. The GPU
// scans EVERY finite logit and keeps the first maximum,including +/-0 ties.
enum { TT_DECODER_GREEDY_WORDS=32,TT_DECODER_GREEDY_MAGIC=0x31524447 };
static inline uint32_t PioneerDecoderLogitKey(uint32_t bits)
{
	if (!(bits&0x7fffffff)) bits=0;
	return bits&0x80000000?~bits:bits^0x80000000;
}
static inline bool PioneerDecoderGreedyValid(const uint32_t* p,unsigned nonce,unsigned tokens)
{
	if (!p || !nonce || nonce>=0x7fffffff || !tokens || tokens>256 || p[0]!=TT_DECODER_GREEDY_MAGIC
		|| p[1]!=nonce || p[2]!=151936 || p[3]>=151936 || (p[4]&0x7f800000)==0x7f800000
		|| p[5]!=tokens || p[6]!=119 || p[7]!=9) return false;
	for (unsigned i=8;i<TT_DECODER_GREEDY_WORDS;i++) if (p[i]) return false;
	return true;
}
#endif
