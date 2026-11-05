/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EXACT_Q4_ORIGINAL_H
#define PIONEER_Q38_EXACT_Q4_ORIGINAL_H
#include "PioneerQ38TwinBatchPrefetchPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
// Exact on-card DATA conversion prerequisite for the full returning worker.
// NO learned dot,FP32 affine,host callback,weight approximation or fallback.
// Caller owes the SAME authenticated original2304B Q4 tile and immutable
// validated native input. Pure values/geometry do NOT grant memory/NoC IO.
struct PioneerQ38ExactQ4Original {
 struct Four {unsigned primary,escape;};
 // Four byte-major original quant values ->four packed SM3 values plus
 // four escape bits. q=z+7+escape EXACTLY,not a shifted affine minimum.
 static Four EncodeFour(unsigned raw,unsigned shift){
  unsigned q=(raw>>shift)&0x0f0f0f0f;
  const unsigned escape=((q+0x01010101)>>4)&0x01010101;
  q-=escape; // q15 ->14,without any inter-byte borrow
  const unsigned biased=(q|0x80808080)-0x07070707;
  const unsigned negative=(~biased>>7)&0x01010101;
  const unsigned flip=(negative<<7)-negative; // independent0x7f masks
  const unsigned magnitude=((biased&0x7f7f7f7f)^flip)+negative;
  unsigned sm=magnitude|(negative<<3);
  sm=(sm|(sm>>4))&0x00ff00ff;sm=(sm|(sm>>8))&65535;
  unsigned mask=(escape|(escape>>7))&0x00030003;mask=(mask|(mask>>14))&15;
  return {sm,mask};
 }
 // Original native operand is0x4000|sign|magnitude,NOT IEEE FP16/BF16.
 // Preserve -128. Canonical integer zero becomes positive FP32/BF16 zero.
 static bool Activation(unsigned value,unsigned& output){
  if(value>65535 || !PioneerInt8OperandValid(uint16_t(value)))return false;
  const unsigned magnitude=value&1023;
  if(!magnitude){output=0;return true;}
  unsigned top=magnitude>=16?4:0;top+=(magnitude>>(top+2))?2:0;top+=(magnitude>>(top+1))?1:0;
  output=(value&0x8000)|((127+top)<<7)|((magnitude<<(7-top))&127);return true;
 }
 static unsigned PanelByteOffset(unsigned panel){return panel<16?256+(panel/4)*512+(panel%2)*256:~0U;}
 // E supplies ONLY bounded ORIGINAL reads and owned primary/mask stores.
 // ALL input validation occurs before any access for invalid geometry.
 template<class E> static bool Panel(E& e,unsigned rawBase,unsigned panel){
  if(panel>=16 || !PioneerQ38TwinBatchPrefetchPlan::RawInput(12,rawBase))return false;
  const unsigned base=rawBase+PanelByteOffset(panel),shift=panel&2?4:0;
  for(unsigned group=0;group<8;group++){
   unsigned mask=0;
   for(unsigned pair=0;pair<4;pair++){
    const unsigned word=group*8+pair*2;
    const Four lo=EncodeFour(e.OriginalWord(base+word*4),shift),hi=EncodeFour(e.OriginalWord(base+(word+1)*4),shift);
    e.Primary(group*4+pair,lo.primary|(hi.primary<<16));
    mask|=lo.escape<<(pair*8);mask|=hi.escape<<(pair*8+4);
   }
   e.MaskWord(group,mask);
  }
  e.SealData();return true;
 }
};
#endif
