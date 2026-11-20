/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PANEL_H
#define PIONEER_QWEN35_DATAFLOW_PANEL_H

#ifndef PIONEER_QWEN35_DATAFLOW_PANEL_TEST
#error Q5 scalar panel repacking is an offline reference, not a Work inference operation
#endif

#include <stddef.h>
#include "PioneerQwen35DataflowProgram.h"
#include "PioneerQwen38Panel.h"

// Offline reference only: Q5_K operands fit the Tensix integer-overlay route
// when represented in Q8_0 panel-zero byte order. Work compute must generate
// this representation with device Tensix operations, not this scalar routine.
// The Q5 affine terms remain exact FP32 metadata for the SFPU path.
struct PioneerQwen35DataflowQ5Panel {
	uint32_t q8Tile[576/4];
	uint32_t coefficient[16];
	uint32_t minimum[16];
};
static_assert(sizeof(PioneerQwen35DataflowQ5Panel)==704,
	"one Q5 compute panel has a fixed 576-byte Tensix tile and 128-byte affine tail");
static_assert(offsetof(PioneerQwen35DataflowQ5Panel,q8Tile)==0
	&& offsetof(PioneerQwen35DataflowQ5Panel,coefficient)
		==PioneerQwen35::DataflowQ5PanelTileBytes
	&& offsetof(PioneerQwen35DataflowQ5Panel,minimum)
		==PioneerQwen35::DataflowQ5PanelTileBytes
			+PioneerQwen35::DataflowAffineMetadataBytes
	&& PioneerQwen35::DataflowQ5PanelScratchAddress
		+offsetof(PioneerQwen35DataflowQ5Panel,coefficient)
		==PioneerQwen35::DataflowAffineCoefficientAddress
	&& PioneerQwen35::DataflowQ5PanelScratchAddress
		+offsetof(PioneerQwen35DataflowQ5Panel,minimum)
		==PioneerQwen35::DataflowAffineMinimumAddress,
	"Q5 panel staging places FP32 affine metadata at the established Tensix addresses");

static inline bool PioneerQwen35PackDataflowQ5Panel(const uint8_t* tile,
	unsigned bytes,unsigned panel,PioneerQwen35DataflowQ5Panel* output)
{
	constexpr unsigned TileBytes=16*176;
	if (!tile || !output || bytes!=TileBytes || panel>=16) return false;
	const uintptr_t source=reinterpret_cast<uintptr_t>(tile);
	const uintptr_t destination=reinterpret_cast<uintptr_t>(output);
	const uintptr_t limit=~uintptr_t(0);
	if ((destination&3) || source>limit-bytes
		|| destination>limit-sizeof(*output)
		|| !(source+bytes<=destination
			|| destination+sizeof(*output)<=source)) return false;

	const unsigned group=panel/2;
	// Validate all affine metadata before publishing any output. The source
	// page is owned by this compute invocation until its CB pop, so the second
	// pass sees the same packed bytes without a 640-byte panel stack object.
	for (unsigned row=0;row<16;row++) {
		unsigned factor,minimum;
		if (group<4) {
			factor=PioneerQ38PanelByte(tile,(4+group)*16+row)&63;
			minimum=PioneerQ38PanelByte(tile,(8+group)*16+row)&63;
		} else {
			factor=(PioneerQ38PanelByte(tile,(8+group)*16+row)&15)
				|((PioneerQ38PanelByte(tile,group*16+row)>>6)<<4);
			minimum=(PioneerQ38PanelByte(tile,(8+group)*16+row)>>4)
				|((PioneerQ38PanelByte(tile,(4+group)*16+row)>>6)<<4);
		}
		const uint16_t d=uint16_t(PioneerQ38PanelByte(tile,row)
			|(PioneerQ38PanelByte(tile,16+row)<<8));
		const uint16_t m=uint16_t(PioneerQ38PanelByte(tile,32+row)
			|(PioneerQ38PanelByte(tile,48+row)<<8));
		uint32_t coefficient,minimumScale;
		if (!PioneerQ38PanelScale(d,int(factor),coefficient)
			|| !PioneerQ38PanelScale(m,-int(minimum),minimumScale)) return false;
	}
	for (unsigned lane=0;lane<16;lane++) for (unsigned row=0;row<16;row++) {
		const unsigned input=panel*16+lane,inputGroup=input/32;
		const unsigned low=PioneerQ38PanelByte(tile,
			(48+(inputGroup/2)*32+input%32)*16+row);
		const unsigned high=PioneerQ38PanelByte(tile,
			(16+input%32)*16+row);
		const unsigned value=((inputGroup&1)?low>>4:low&15)
			|(((high>>inputGroup)&1)<<4);
		if (value>31) return false;
	}

	for (unsigned word=0;word<576/4;word++) output->q8Tile[word]=0;
	for (unsigned lane=0;lane<16;lane++) for (unsigned row=0;row<16;row++) {
		const unsigned input=panel*16+lane,inputGroup=input/32;
		const unsigned low=PioneerQ38PanelByte(tile,
			(48+(inputGroup/2)*32+input%32)*16+row);
		const unsigned high=PioneerQ38PanelByte(tile,
			(16+input%32)*16+row);
		const unsigned value=((inputGroup&1)?low>>4:low&15)
			|(((high>>inputGroup)&1)<<4);
		const unsigned byte=(2+lane)*16+row;
		output->q8Tile[byte/4]|=uint32_t(value)<<((byte&3)*8);
	}
	for (unsigned row=0;row<16;row++) {
		unsigned factor,minimum;
		if (group<4) {
			factor=PioneerQ38PanelByte(tile,(4+group)*16+row)&63;
			minimum=PioneerQ38PanelByte(tile,(8+group)*16+row)&63;
		} else {
			factor=(PioneerQ38PanelByte(tile,(8+group)*16+row)&15)
				|((PioneerQ38PanelByte(tile,group*16+row)>>6)<<4);
			minimum=(PioneerQ38PanelByte(tile,(8+group)*16+row)>>4)
				|((PioneerQ38PanelByte(tile,(4+group)*16+row)>>6)<<4);
		}
		const uint16_t d=uint16_t(PioneerQ38PanelByte(tile,row)
			|(PioneerQ38PanelByte(tile,16+row)<<8));
		const uint16_t m=uint16_t(PioneerQ38PanelByte(tile,32+row)
			|(PioneerQ38PanelByte(tile,48+row)<<8));
		if (!PioneerQ38PanelScale(d,int(factor),output->coefficient[row])
			|| !PioneerQ38PanelScale(m,-int(minimum),output->minimum[row])) return false;
	}
	return true;
}

#endif
