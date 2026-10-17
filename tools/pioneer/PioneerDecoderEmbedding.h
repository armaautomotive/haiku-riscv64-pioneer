/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DECODER_EMBEDDING_H
#define PIONEER_DECODER_EMBEDDING_H
#include "PioneerAttentionMath.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDecoderLayout.h"
// OFFLINE prerequisite for autonomous on-card greedy generation. The
// fixed Qwen3-0.6B model ties token embeddings to its sealed classifier.
// No new driver operation,weight write,shape,address selection or fallback.
// A future owned image must derive token from its proved local argmax and
// base from the immutable registry,not expose this as an arbitrary reader.
template<class IO>
static bool PioneerDecoderEmbedding(IO& io,unsigned token,uint32_t vocabularyBase,float* output)
{
	if (!output || token>=151936 || !PioneerDecoderWeight(vocabularyBase,1024,151936)) return false;
	const unsigned row=token&15;
	// Native FP32-scale packing:32 K-blocks per16-row tile;each has
	// 16 exact half-derived FP32 scales then32x16 sign-magnitude bytes.
	const uint32_t tile=vocabularyBase+(token>>4)*18432;
	for (unsigned block=0;block<32;block++) {
		uint32_t first[144],second[144];
		if (!io.ReadNativeEmbeddingBlock(tile+block*576,first)
			|| !io.ReadNativeEmbeddingBlock(tile+block*576,second)) return false;
		for (unsigned i=0;i<144;i++) if (first[i]!=second[i]) return false;
		float scale=PioneerAttentionFloat(first[row]);
		if ((first[row]&0x7fffffff)>0x477fe000
			|| PioneerAttentionBits(PioneerAttentionHalf(PioneerAttentionToHalf(scale)))!=first[row]) return false;
		for (unsigned i=0;i<32;i++) {
			unsigned byte=64+i*16+row,encoded=(first[byte>>2]>>((byte&3)*8))&255;
			if (encoded==128) return false; // noncanonical -0 cannot be emitted by the sealed Q8 importer
			int integer=int(encoded&127); if (encoded&128) integer=-integer;
			float value=float(integer)*scale;
			uint32_t absolute=PioneerAttentionBits(value)&0x7fffffff;
			if (absolute>0x477fe000 || (absolute && absolute<0x26800000)) return false;
			output[block*32+i]=value;
		}
	}
	return true;
}
#endif
