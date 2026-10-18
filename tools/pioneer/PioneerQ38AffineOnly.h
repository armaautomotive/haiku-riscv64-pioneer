/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_AFFINE_ONLY_H
#define PIONEER_Q38_AFFINE_ONLY_H
#include "PioneerQ38BinaryScale.h"
// Future vector-weight path ONLY:decode exact affine metadata without
// decoding,zeroing or publishing ANY scalar matrix operands. Existing
// qualified images remain unchanged and do not include/select this helper.
static inline bool PioneerQ38AffineOnly(unsigned type,const uint8_t* tile,unsigned bytes,unsigned panel,PioneerQ38Panel* output)
{
	unsigned required=PioneerQ38PanelTileBytes(type),panels=type==8?2:16;
	if (!required || !tile || !output || bytes!=required || panel>=panels) return false;
	uintptr_t first=reinterpret_cast<uintptr_t>(tile),last=reinterpret_cast<uintptr_t>(output),limit=~uintptr_t(0);
	if ((last&3) || first>limit-bytes || last>limit-sizeof(*output)
		|| !(first+bytes<=last || last+sizeof(*output)<=first)) return false;
	uint32_t coefficients[16],minima[16];
	for (unsigned row=0;row<16;row++) {
		minima[row]=0;
		unsigned at=type==14?208:0;
		uint16_t half=uint16_t(PioneerQ38PanelByte(tile,at*16+row)|(PioneerQ38PanelByte(tile,(at+1)*16+row)<<8));
		int factor=1;
		if (type==12) {
			unsigned group=panel/2,minimum;
			if (group<4) { factor=PioneerQ38PanelByte(tile,(4+group)*16+row)&63; minimum=PioneerQ38PanelByte(tile,(8+group)*16+row)&63; }
			else { factor=int((PioneerQ38PanelByte(tile,(8+group)*16+row)&15)|((PioneerQ38PanelByte(tile,group*16+row)>>6)<<4));
				minimum=(PioneerQ38PanelByte(tile,(8+group)*16+row)>>4)|((PioneerQ38PanelByte(tile,(4+group)*16+row)>>6)<<4); }
			uint16_t minHalf=uint16_t(PioneerQ38PanelByte(tile,32+row)|(PioneerQ38PanelByte(tile,48+row)<<8));
			if (!PioneerQ38BinaryScale(minHalf,-int(minimum),minima[row])) return false;
		} else if (type==14) {
			unsigned value=PioneerQ38PanelByte(tile,(192+panel)*16+row); factor=int(value)-int((value&128)*2);
		}
		if (!PioneerQ38BinaryScale(half,factor,coefficients[row])) return false;
	}
	// Atomic metadata publication after ALL rows validate;operand remains
	// caller-owned and untouched on success AND failure.
	for (unsigned row=0;row<16;row++) { output->coefficient[row]=coefficients[row]; output->minimum[row]=minima[row]; }
	return true;
}
#endif
