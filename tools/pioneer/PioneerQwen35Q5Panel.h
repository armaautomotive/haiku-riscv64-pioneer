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
 PioneerQ38Panel stage;
 volatile uint32_t* zero=stage.operand;
 for (unsigned w=0;w<128;w++) zero[w]=0;
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
  if (!PioneerQ38PanelScale(d,int(factor),stage.coefficient[row])
   || !PioneerQ38PanelScale(m,-int(minimum),stage.minimum[row])) return false;
  for (unsigned lane=0;lane<16;lane++) {
   unsigned i=panel*16+lane,g=i/32;
   unsigned lo=PioneerQ38PanelByte(tile,(48+(g/2)*32+i%32)*16+row);
   unsigned hi=PioneerQ38PanelByte(tile,(16+i%32)*16+row);
   unsigned q=((g&1)?lo>>4:lo&15) | (((hi>>g)&1)<<4);
   unsigned element=lane*16+row;
   stage.operand[element/2]|=uint32_t(PioneerInt8Operand(int(q)))<<((element%2)*16);
  }
 }
 for (unsigned w=0;w<128;w++) output->operand[w]=stage.operand[w];
 for (unsigned row=0;row<16;row++) {
  output->coefficient[row]=stage.coefficient[row]; output->minimum[row]=stage.minimum[row];
 }
 return true;
}
#endif
