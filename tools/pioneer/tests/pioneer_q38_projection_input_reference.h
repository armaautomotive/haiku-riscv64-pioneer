/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_INPUT_REFERENCE_H
#define PIONEER_Q38_PROJECTION_INPUT_REFERENCE_H
#include "pioneer_qwen38_activation_reference.h"
#include "../PioneerQ38ProjectionInput.h"
// TEST ONLY:independent IEEE/half/rounding and explicit native bit packing.
static void ProjectionInputReference(unsigned type,const float* values,PioneerQ38ProjectionInput& out){
 memset(&out,0,sizeof(out));
 if(type==8){
  for(unsigned block=0;block<8;block++){
   uint32_t q[9];Q38Activation0Reference(values+block*32,q);
   uint16_t encoded=uint16_t(q[0]);_Float16 half;memcpy(&half,&encoded,2);
   out.scale[block]=PioneerAttentionBits(float(half));
   for(unsigned lane=0;lane<32;lane++){
    unsigned byte=lane+2,raw=(q[byte/4]>>((byte%4)*8))&255;int n=int(raw)-int((raw&128)*2);
    unsigned at=block*32+lane;out.operand[at/2]|=(0x4000U|(n<0?0x8000U|unsigned(-n):unsigned(n)))<<((at%2)*16);
   }
  }
 }else{
  uint32_t q[73];Q38ActivationReference(values,q);out.scale[0]=q[0];
  for(unsigned lane=0;lane<256;lane++){
   unsigned raw=(q[1+lane/4]>>((lane%4)*8))&255;int n=int(raw)-int((raw&128)*2);
   out.operand[lane/2]|=(0x4000U|(n<0?0x8000U|unsigned(-n):unsigned(n)))<<((lane%2)*16);
  }
 }
}
#endif
