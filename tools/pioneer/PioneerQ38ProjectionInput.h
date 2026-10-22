/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_INPUT_H
#define PIONEER_Q38_PROJECTION_INPUT_H
#include "PioneerQ38FfnNativeInput.h"
// Local arithmetic ONLY for the future SAME persistent projection family.
// One256-F32 chunk becomes the existing512B native operands plus a64B
// scale vector:Q8_0 has EIGHT positive FP16-rounded scales,Q4_K/Q6_K ONE
// signed F32 scale. No addresses,source/producer/initialized/lifetime or
// native execution authority. No learned product or CPU fallback.
struct PioneerQ38ProjectionInput {uint32_t operand[128],scale[16];};
static_assert(sizeof(PioneerQ38ProjectionInput)==576,"fixed native chunk/padded scales");
static __attribute__((noinline)) bool PioneerQ38ProjectionQuantize(unsigned type,const float* input,unsigned count,
 PioneerQ38ProjectionInput* output)
{
 if((type!=8 && type!=12 && type!=14) || count!=256 || !input || !output
  || (reinterpret_cast<__UINTPTR_TYPE__>(input)&3) || (reinterpret_cast<__UINTPTR_TYPE__>(output)&3)
  || !PioneerQ38Disjoint(input,256,reinterpret_cast<const float*>(output),144))return false;
 PioneerQ38ProjectionInput stage;
 for(unsigned i=0;i<128;i++)stage.operand[i]=0;
 for(unsigned i=0;i<16;i++)stage.scale[i]=0;
 if(type==8){
  for(unsigned block=0;block<8;block++){
   PioneerQ38Activation0 q;if(!PioneerQ38Quantize0(input+block*32,32,&q))return false;
   const uint32_t scale=PioneerAttentionBits(PioneerAttentionHalf(uint16_t(q.words[0]))),magnitude=scale&0x7fffffff;
   if((scale&0x80000000U) || (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)))return false;
   stage.scale[block]=scale;
   for(unsigned lane=0;lane<32;lane++){
    unsigned byte=lane+2,raw=(q.words[byte/4]>>((byte%4)*8))&255;
    int value=int(raw)-int((raw&128)*2);if(value==-128)return false;
    unsigned at=block*32+lane;stage.operand[at/2]|=uint32_t(PioneerInt8Operand(value))<<((at%2)*16);
   }
  }
  // A nonzero Q8_0 vector may round its FP16 scale to ZERO. Preserve that
  // exact existing wire behavior;Q8_K's zero-scale=>zero-lanes rule does
  // NOT apply to this different quantizer.
 }else{
  PioneerQ38ActivationK q;PioneerQ38FfnNativeInput native;
  if(!PioneerQ38QuantizeK(input,256,&q) || !PioneerQ38FfnPackNative(&q,&native))return false;
  for(unsigned i=0;i<128;i++)stage.operand[i]=native.operand[i];
  for(unsigned i=0;i<16;i++)stage.scale[i]=native.scale[i];
 }
 // Atomic publication ONLY after ALL eight Q8_0 blocks or the complete
 // Q8_K block passed. Never leak a partial late-block result.
 for(unsigned i=0;i<128;i++)output->operand[i]=stage.operand[i];
 for(unsigned i=0;i<16;i++)output->scale[i]=stage.scale[i];
 return true;
}
#endif
