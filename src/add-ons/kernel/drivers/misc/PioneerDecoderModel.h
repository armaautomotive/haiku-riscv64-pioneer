/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DECODER_MODEL_H
#define PIONEER_DECODER_MODEL_H
// Private fixed-model immutable registration;control/host fields are derived.
#include <stdint.h>
#include "PioneerDecoderLayout.h"
struct PioneerDecoderLayerOffsets { uint64_t q,v,k,o,gate,up,down,attentionNorm,ffnNorm,qNorm,kNorm; };
struct PioneerDecoderModelOffsets { PioneerDecoderLayerOffsets layers[28]; uint64_t finalNorm,vocabulary; };
static inline bool PioneerDecoderModelRange(uint64_t offset,uint64_t bytes,const uint8_t* initialized,uint32_t& address)
{
	const uint64_t end=UINT64_C(0x80000000)-28*1024*1024;
	if (!initialized || (offset&4095) || !bytes || offset>=end || bytes>end-offset) return false;
	for (uint64_t p=offset/4096;p<(offset+bytes+4095)/4096;p++) if (initialized[p]!=1) return false;
	address=0x04000000+uint32_t(offset); return true;
}
static inline bool PioneerDecoderModelRegister(const PioneerDecoderModelOffsets& offsets,const uint8_t* initialized,
	uint64_t capacity,PioneerDecoderJob& job)
{
	job={};
	if (capacity!=UINT64_C(0x80000000) || !initialized) return false;
	// The private mutable KV tail must not contain initialized learned data.
	for (uint64_t p=(capacity-28*1024*1024)/4096;p<capacity/4096;p++) if (initialized[p]) return false;
	for (unsigned layer=0;layer<28;layer++) {
		const auto* in=reinterpret_cast<const uint64_t*>(&offsets.layers[layer]); auto* out=reinterpret_cast<uint32_t*>(&job.layers[layer]);
		const unsigned k[]={1024,1024,1024,2048,1024,1024,3072},n[]={2048,1024,1024,1024,3072,3072,1024};
		for (unsigned i=0;i<11;i++) {
			uint64_t bytes=i<7?uint64_t(k[i])*n[i]*9/8:4096;
			if (!PioneerDecoderModelRange(in[i],bytes,initialized,out[i])) { job={}; return false; }
		}
	}
	if (!PioneerDecoderModelRange(offsets.finalNorm,4096,initialized,job.finalNorm)
		|| !PioneerDecoderModelRange(offsets.vocabulary,UINT64_C(1024)*151936*9/8,initialized,job.vocabulary)) { job={}; return false; }
	// Runtime control/nonce/sequence/host fields remain ZERO. A dedicated
	// controller derives them from its exclusive private owners per call.
	return true;
}
#endif
