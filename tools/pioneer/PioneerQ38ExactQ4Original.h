/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EXACT_Q4_ORIGINAL_H
#define PIONEER_Q38_EXACT_Q4_ORIGINAL_H
#include "PioneerQ38TwinBatchPrefetchPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
// Experimental exact activation lookup. Its only purpose is to trade a small
// local constant table for repeated integer leading-bit normalization in
// PreparedQ4Inputs; the default worker keeps the original arithmetic path.
// The explicit table is C++11-compatible (the target's required language mode).
static const unsigned short kPioneerQ38ExactQ4ActivationLookup[129] = {
	0x0000,0x3f80,0x4000,0x4040,0x4080,0x40a0,0x40c0,0x40e0,
	0x4100,0x4110,0x4120,0x4130,0x4140,0x4150,0x4160,0x4170,
	0x4180,0x4188,0x4190,0x4198,0x41a0,0x41a8,0x41b0,0x41b8,
	0x41c0,0x41c8,0x41d0,0x41d8,0x41e0,0x41e8,0x41f0,0x41f8,
	0x4200,0x4204,0x4208,0x420c,0x4210,0x4214,0x4218,0x421c,
	0x4220,0x4224,0x4228,0x422c,0x4230,0x4234,0x4238,0x423c,
	0x4240,0x4244,0x4248,0x424c,0x4250,0x4254,0x4258,0x425c,
	0x4260,0x4264,0x4268,0x426c,0x4270,0x4274,0x4278,0x427c,
	0x4280,0x4282,0x4284,0x4286,0x4288,0x428a,0x428c,0x428e,
	0x4290,0x4292,0x4294,0x4296,0x4298,0x429a,0x429c,0x429e,
	0x42a0,0x42a2,0x42a4,0x42a6,0x42a8,0x42aa,0x42ac,0x42ae,
	0x42b0,0x42b2,0x42b4,0x42b6,0x42b8,0x42ba,0x42bc,0x42be,
	0x42c0,0x42c2,0x42c4,0x42c6,0x42c8,0x42ca,0x42cc,0x42ce,
	0x42d0,0x42d2,0x42d4,0x42d6,0x42d8,0x42da,0x42dc,0x42de,
	0x42e0,0x42e2,0x42e4,0x42e6,0x42e8,0x42ea,0x42ec,0x42ee,
	0x42f0,0x42f2,0x42f4,0x42f6,0x42f8,0x42fa,0x42fc,0x42fe,
	0x4300
};
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
 static bool ActivationLookup(unsigned value,unsigned& output){
  if(value>65535 || !PioneerInt8OperandValid(uint16_t(value)))return false;
  const unsigned magnitude=value&1023;
  output=kPioneerQ38ExactQ4ActivationLookup[magnitude]
	| (magnitude ? value&0x8000 : 0);return true;
 }
 static bool ActivationByteLookup(unsigned value,unsigned& output){
  // Encoded activations have eight magnitude bits: bits10..14 are the fixed
  // exponent and bit15 is sign. Validate before indexing, preserving the
  // asymmetric +127/-128 limits without testing unused upper magnitude bits.
  if(value>65535)return false;
  const unsigned magnitude=value&255,negative=value&0x8000;
  if((value&0x7f00)!=0x4000 || magnitude>(negative?128U:127U))return false;
  output=kPioneerQ38ExactQ4ActivationLookup[magnitude]
	| (magnitude ? negative : 0);return true;
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
