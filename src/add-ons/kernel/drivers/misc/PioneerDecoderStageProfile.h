/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DECODER_STAGE_PROFILE_H
#define PIONEER_DECODER_STAGE_PROFILE_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// Diagnostic ONLY: device clock ticks,NOT host microseconds or throughput.
// The fixed trace lies inside the existing backed-up bulk owner,147800.
enum { TT_DECODER_STAGE_L1=0x147800,TT_DECODER_STAGE_WORDS=32,TT_DECODER_STAGES=8 };
struct tt_decoder_stage_profile {
	uint32_t magic,nonce,tokens,version,cycles[8],calls[8],reserved[12];
};
static_assert(sizeof(tt_decoder_stage_profile)==128,"fixed data-only trace");
static inline bool PioneerDecoderStageValid(const tt_decoder_stage_profile& p,unsigned nonce,unsigned tokens,unsigned version=1)
{
	if ((version<1 || version>4) || p.magic!=0x31525044 || p.version!=version || p.nonce!=nonce || !nonce || p.tokens!=tokens || !tokens || tokens>256) return false;
	const unsigned counts[]={version>=3?168U:196U,version==4?57U:113U,version>=2?85U:113U,version==4?0U:28U,28,28,1,1}; uint32_t sum=0;
	for (unsigned i=0;i<8;i++) {
		if (p.calls[i]!=counts[i] || (counts[i]?!p.cycles[i]:p.cycles[i]!=0)) return false;
		if (i<7) { if (p.cycles[i]>uint32_t(-1)-sum) return false; sum+=p.cycles[i]; }
	}
	for (unsigned i=0;i<12;i++) if (p.reserved[i]) return false;
	return sum<=p.cycles[7]; // Reject wrap/invalid diagnostic;never an inference gate.
}
#endif
