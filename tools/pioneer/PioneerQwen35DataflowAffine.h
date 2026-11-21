/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_AFFINE_H
#define PIONEER_QWEN35_DATAFLOW_AFFINE_H

#include "PioneerQ38AffineOnly.h"
#include "PioneerQ38BinaryScale.h"
#include "PioneerQwen35DataflowProgram.h"

// Q5_K has the same d/dmin/scales prefix as Q4_K, with its qh/qs planes
// following at byte offsets 16 and 48. Read only the 16 affine factors here;
// packed operands and the projection dot are handled by Tensix.
static bool PioneerQwen35DataflowQ5AffineOnly(const uint8_t* tile,
	unsigned bytes,unsigned panel,PioneerQ38Panel* output)
{
	if (!tile || !output || bytes!=16*176 || panel>=16) return false;
	const uintptr_t first=reinterpret_cast<uintptr_t>(tile);
	const uintptr_t last=reinterpret_cast<uintptr_t>(output);
	const uintptr_t limit=~uintptr_t(0);
	if ((last&3) || first>limit-bytes || last>limit-sizeof(*output)
		|| !(first+bytes<=last || last+sizeof(*output)<=first)) return false;
	uint32_t coefficient[16],minimumScale[16];
	const unsigned group=panel/2;
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
		const uint16_t dmin=uint16_t(PioneerQ38PanelByte(tile,32+row)
			|(PioneerQ38PanelByte(tile,48+row)<<8));
		if (!PioneerQ38BinaryScale(d,int(factor),coefficient[row])
			|| !PioneerQ38BinaryScale(dmin,-int(minimum),minimumScale[row]))
			return false;
	}
	for (unsigned row=0;row<16;row++) {
		output->coefficient[row]=coefficient[row];
		output->minimum[row]=minimumScale[row];
	}
	return true;
}

static bool PioneerQwen35DataflowAffineOnly(unsigned type,const uint8_t* tile,
	unsigned bytes,unsigned panel,PioneerQ38Panel* output)
{
	if (type==13)
		return PioneerQwen35DataflowQ5AffineOnly(tile,bytes,panel,output);
	return PioneerQ38AffineOnly(type,tile,bytes,panel,output);
}

#endif
