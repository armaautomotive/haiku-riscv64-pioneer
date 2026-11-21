/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstring>

#include "../PioneerAttentionMath.h"
#include "../PioneerQwen35DataflowEmbedding.h"
#include "../PioneerQwen35Embedding.h"

using namespace PioneerQwen35;

int main()
{
	uint8_t tile[DataflowQ6KEmbeddingTileBytes];
	for (unsigned i=0;i<sizeof(tile);i++)
		tile[i]=uint8_t((i*73U+(i>>3)*19U+11U)&255U);
	for (unsigned lane=0;lane<16;lane++) {
		const uint16_t half=uint16_t(0x3000U+(lane*0x31U));
		tile[208*16+lane]=uint8_t(half);
		tile[209*16+lane]=uint8_t(half>>8);
	}
	float reference[256],decoded[16];
	for (unsigned lane=0;lane<16;lane++) {
		assert(PioneerQwen35Embedding::DecodeBlock(tile,sizeof(tile),lane,
			reference,256));
		for (unsigned panel=0;panel<DataflowQ6KEmbeddingPanels;panel++) {
			assert(DataflowDecodeQ6KEmbeddingPanel(tile,sizeof(tile),lane,panel,
				decoded,16));
			for (unsigned value=0;value<16;value++)
				assert(PioneerAttentionBits(decoded[value])
					==PioneerAttentionBits(reference[panel*16+value]));
		}
	}
	assert(!DataflowDecodeQ6KEmbeddingPanel(tile,sizeof(tile),16,0,decoded,16));
	assert(!DataflowDecodeQ6KEmbeddingPanel(tile,sizeof(tile),0,16,decoded,16));
	assert(!DataflowDecodeQ6KEmbeddingPanel(tile,sizeof(tile)-1,0,0,decoded,16));
	assert(!DataflowDecodeQ6KEmbeddingPanel(tile,sizeof(tile),0,0,decoded,15));
	assert(!DataflowDecodeQ6KEmbeddingPanel(tile,sizeof(tile),0,0,
		reinterpret_cast<float*>(tile),16));
	return 0;
}
