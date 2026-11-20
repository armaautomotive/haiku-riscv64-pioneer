/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_Q5_PANEL_H
#define PIONEER_QWEN35_Q5_PANEL_H
#include "PioneerQwen38Panel.h"

// Offline validation primitive only. Input is 16 original Q5_K blocks in
// byte-major order, with no padding (176 bytes per row). Output has the
// existing Tensix integer-overlay panel layout and exact affine metadata.
// This does not register a kernel, authorize a card session, or perform a dot.
static inline bool PioneerQwen35Q5Panel(const uint8_t* tile,unsigned bytes,
 unsigned panel,PioneerQ38Panel* output)
{
 constexpr unsigned tileBytes=16*176;
 if (!tile || !output || bytes!=tileBytes || panel>=16) return false;
 uintptr_t first=reinterpret_cast<uintptr_t>(tile),last=reinterpret_cast<uintptr_t>(output),limit=~uintptr_t(0);
 if ((last&3) || first>limit-bytes || last>limit-sizeof(*output)
  || !(first+bytes<=last || last+sizeof(*output)<=first)) return false;
	uint32_t coefficients[16],minima[16];
 for (unsigned row=0;row<16;row++) {
  unsigned group=panel/2;
  unsigned factor,minimum;
  if (group<4) {
   factor=PioneerQ38PanelByte(tile,(4+group)*16+row)&63;
   minimum=PioneerQ38PanelByte(tile,(8+group)*16+row)&63;
  } else {
   factor=(PioneerQ38PanelByte(tile,(8+group)*16+row)&15)
    | ((PioneerQ38PanelByte(tile,group*16+row)>>6)<<4);
   minimum=(PioneerQ38PanelByte(tile,(8+group)*16+row)>>4)
    | ((PioneerQ38PanelByte(tile,(4+group)*16+row)>>6)<<4);
  }
  uint16_t d=uint16_t(PioneerQ38PanelByte(tile,row)|(PioneerQ38PanelByte(tile,16+row)<<8));
  uint16_t m=uint16_t(PioneerQ38PanelByte(tile,32+row)|(PioneerQ38PanelByte(tile,48+row)<<8));
		if (!PioneerQ38PanelScale(d,int(factor),coefficients[row])
			|| !PioneerQ38PanelScale(m,-int(minimum),minima[row])) return false;
 }
	// Fill each packed operand word once. Adjacent rows share the two
	// halfwords, so no zero pass or read/modify/write OR is needed.
	for (unsigned lane=0;lane<16;lane++) for (unsigned row=0;row<16;row+=2) {
		unsigned i0=panel*16+lane,g0=i0/32;
		unsigned lo0=PioneerQ38PanelByte(tile,(48+(g0/2)*32+i0%32)*16+row);
		unsigned hi0=PioneerQ38PanelByte(tile,(16+i0%32)*16+row);
		unsigned q0=((g0&1)?lo0>>4:lo0&15) | (((hi0>>g0)&1)<<4);
		unsigned lo1=PioneerQ38PanelByte(tile,(48+(g0/2)*32+i0%32)*16+row+1);
		unsigned hi1=PioneerQ38PanelByte(tile,(16+i0%32)*16+row+1);
		unsigned q1=((g0&1)?lo1>>4:lo1&15) | (((hi1>>g0)&1)<<4);
		output->operand[lane*8+row/2]=uint32_t(PioneerInt8Operand(int(q0)))
			|(uint32_t(PioneerInt8Operand(int(q1)))<<16);
	}
 for (unsigned row=0;row<16;row++) {
		output->coefficient[row]=coefficients[row]; output->minimum[row]=minima[row];
 }
 return true;
}
#endif
