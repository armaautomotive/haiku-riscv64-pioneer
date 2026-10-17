/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DECODER_BATCH_MODEL_H
#define PIONEER_DECODER_BATCH_MODEL_H
#include "PioneerDecoderModel.h"
// Explicit experimental model17/contract33 registration prerequisite.
// The32-page immutable256-position RoPE table must be initialized and
// entirely disjoint from EVERY registered weight/norm and mutable KV tail.
static inline bool PioneerDecoderBatchModelRegister(const PioneerDecoderModelOffsets& offsets,
	uint64_t ropeOffset,unsigned eos0,unsigned eos1,const uint8_t* initialized,uint64_t capacity,PioneerDecoderJob& job)
{
	if (!PioneerDecoderModelRegister(offsets,initialized,capacity,job)) return false;
	uint32_t rope=0;
	bool valid=PioneerDecoderEogSetValid(eos0,eos1)
		&& PioneerDecoderModelRange(ropeOffset,0x20000,initialized,rope);
	const unsigned k[]={1024,1024,1024,2048,1024,1024,3072},n[]={2048,1024,1024,1024,3072,3072,1024};
	for (unsigned l=0;l<28 && valid;l++) for (unsigned i=0;i<11;i++) {
		const uint32_t* words=reinterpret_cast<const uint32_t*>(&job.layers[l]);
		unsigned bytes=i<7?k[i]*n[i]*9/8:4096;
		valid &= rope+0x20000<=words[i] || words[i]+bytes<=rope;
	}
	if (valid) valid=(rope+0x20000<=job.finalNorm || job.finalNorm+4096<=rope)
		&& (rope+0x20000<=job.vocabulary || job.vocabulary+175030272<=rope);
	if (!valid) { job={}; return false; }
	job.reserved[1]=rope; job.reserved[2]=eos0; job.reserved[3]=eos1;
	return true;
}
#endif
