/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_PANEL_H
#define PIONEER_Q38_VECTOR_PANEL_H
#include "PioneerQ38VectorOperand.h"
// OFFLINE fixed-layout prerequisite. NO native image/handler selects it.
// The raw planes each contain sixteen consecutive16-byte row slices from
// the original lossless tiled block. These are DATA OFFSETS,not addresses.
struct PioneerQ38VectorPanelSpan { unsigned low=0,high=0; bool second=false; };
static inline bool PioneerQ38VectorPanel(unsigned type,unsigned panel,PioneerQ38VectorPanelSpan& result)
{
	if ((type!=8 && type!=12 && type!=14) || panel>=(type==8?2U:16U)) return false;
	PioneerQ38VectorPanelSpan s;
	if (type==8) s.low=(2+panel*16)*16;
	else if (type==12) s.low=(16+(panel/4)*32+(panel%2)*16)*16;
	else {
		unsigned half=panel/8,plane=(panel%8)/2;
		s.low=(half*64+(plane%2)*32+(panel%2)*16)*16;
		s.high=(128+half*32+(panel%2)*16)*16; s.second=true;
	}
	unsigned bytes=type==8?576:type==12?2304:3392;
	if ((s.low&15) || s.low>bytes-256 || (s.second && ((s.high&15) || s.high>bytes-256))) return false;
	result=s; return true;
}
#endif
