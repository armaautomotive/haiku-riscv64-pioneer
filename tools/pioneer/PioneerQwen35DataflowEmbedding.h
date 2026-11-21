/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_EMBEDDING_H
#define PIONEER_QWEN35_DATAFLOW_EMBEDDING_H

#include "PioneerAttentionMath.h"
#include "PioneerQwen38Panel.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowQ6KEmbeddingTileBytes=3392,
	DataflowQ6KEmbeddingPanels=16,
	DataflowQ6KEmbeddingValuesPerPanel=16
};

// Decode one 16-value slice from the selected vocabulary lane of a tiled
// Q6_K embedding block. This bounded scalar unpack is intended to run in the
// resident device worker; the host only supplies token and manifest indices.
// Projection arithmetic remains on the Tensix matrix engine.
inline bool DataflowDecodeQ6KEmbeddingPanel(const uint8_t* tile,
	unsigned bytes,unsigned lane,unsigned panel,float* output,unsigned values)
{
	if (!tile || bytes!=DataflowQ6KEmbeddingTileBytes || lane>=16
		|| panel>=DataflowQ6KEmbeddingPanels || !output
		|| values!=DataflowQ6KEmbeddingValuesPerPanel) return false;
	const uintptr_t source=reinterpret_cast<uintptr_t>(tile);
	const uintptr_t destination=reinterpret_cast<uintptr_t>(output);
	constexpr unsigned outputBytes=DataflowQ6KEmbeddingValuesPerPanel*sizeof(float);
	const uintptr_t limit=~uintptr_t(0);
	if ((destination&3) || source>limit-bytes || destination>limit-outputBytes
		|| !(source+bytes<=destination || destination+outputBytes<=source))
		return false;
	const unsigned half=PioneerQ38PanelByte(tile,208*16+lane)
		|(PioneerQ38PanelByte(tile,209*16+lane)<<8);
	const unsigned packedScale=PioneerQ38PanelByte(tile,(192+panel)*16+lane);
	const int scale=int(packedScale)-int((packedScale&128)*2);
	uint32_t coefficient=0;
	if (!PioneerQ38PanelScale(uint16_t(half),scale,coefficient)) return false;
	const float factor=PioneerAttentionFloat(coefficient);
	for (unsigned value=0;value<DataflowQ6KEmbeddingValuesPerPanel;value++) {
		const unsigned element=panel*DataflowQ6KEmbeddingValuesPerPanel+value;
		const unsigned halfIndex=element/128;
		const unsigned plane=(element%128)/32;
		const unsigned column=element%32;
		const unsigned lowByte=(halfIndex*64+(plane&1)*32+column)*16+lane;
		const unsigned highByte=(128+halfIndex*32+column)*16+lane;
		const unsigned low=(PioneerQ38PanelByte(tile,lowByte)
			>>((plane/2)*4))&15;
		const unsigned high=(PioneerQ38PanelByte(tile,highByte)
			>>(plane*2))&3;
		output[value]=factor*float(int(low|(high<<4))-32);
	}
	return true;
}

} // namespace PioneerQwen35

#endif
