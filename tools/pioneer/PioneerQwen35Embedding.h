/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_EMBEDDING_H
#define PIONEER_QWEN35_EMBEDDING_H

#include "PioneerAttentionMath.h"
#include "PioneerQwen38Panel.h"

namespace PioneerQwen35Embedding {
enum : unsigned { Width=2560, Vocabulary=248320, BlockValues=256,
	Blocks=Width/BlockValues, TileBytes=3392, GroupBytes=Blocks*TileBytes };

// Decode only the requested lane from a tiled Q6_K block. The same tiled
// group also feeds the vocabulary projection. The owner supplies the bounded
// transfer and publishes the hidden row after all ten blocks are decoded.
inline bool DecodeBlock(const uint8_t* tile, unsigned bytes, unsigned lane,
	float* output, unsigned values)
{
	if (!tile || bytes!=TileBytes || lane>=16 || !output || values!=BlockValues)
		return false;
	const uintptr_t source=reinterpret_cast<uintptr_t>(tile);
	const uintptr_t destination=reinterpret_cast<uintptr_t>(output);
	constexpr unsigned outputBytes=BlockValues*sizeof(float);
	const uintptr_t limit=~uintptr_t(0);
	if ((destination&3) || source>limit-bytes || destination>limit-outputBytes
		|| !(source+bytes<=destination || destination+outputBytes<=source))
		return false;
	const unsigned d=PioneerQ38PanelByte(tile,208*16+lane)
		| (PioneerQ38PanelByte(tile,209*16+lane)<<8);
	if ((d&0x7c00)==0x7c00)
		return false;
	for (unsigned panel=0;panel<BlockValues/16;panel++) {
		const unsigned packedScale=PioneerQ38PanelByte(tile,(192+panel)*16+lane);
		const int scale=int(packedScale)-int((packedScale&128)*2);
		uint32_t coefficient=0;
		if (!PioneerQ38PanelScale(uint16_t(d),scale,coefficient))
			return false;
		for (unsigned element=panel*16;element<(panel+1)*16;element++) {
			const unsigned half=element/128, plane=(element%128)/32, column=element%32;
			const unsigned lowByte=(half*64+(plane&1)*32+column)*16+lane;
			const unsigned highByte=(128+half*32+column)*16+lane;
			const unsigned low=(PioneerQ38PanelByte(tile,lowByte)>>((plane/2)*4))&15;
			const unsigned high=(PioneerQ38PanelByte(tile,highByte)>>(plane*2))&3;
			output[element]=PioneerAttentionFloat(coefficient)*float(int(low|(high<<4))-32);
		}
	}
	return true;
}
}
#endif
