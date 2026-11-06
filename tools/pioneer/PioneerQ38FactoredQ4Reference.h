/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FACTORED_Q4_REFERENCE_H
#define PIONEER_Q38_FACTORED_Q4_REFERENCE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(__FAST_MATH__)
#error Factored Q4 reference is CPU DIAGNOSTIC ONLY,never native or Work
#endif
#include "PioneerBlackholeBf16Arithmetic.h"
#include "PioneerBlackholeFma.h"
#include "PioneerQ38BinaryScale.h"
// Independent ORIGINAL-byte diagnostic contract. No producer template,
// device/cache/backend/IO or substitution of a CPU answer. This arithmetic
// is the new output-affine order,not old BF16/GGML weight rounding.
struct PioneerQ38FactoredQ4Reference {
 static bool Block(const uint8_t* original,const uint16_t* input,uint32_t* accumulator){
  if(!original || !input || !accumulator)return false;
  for(unsigned group=0;group<8;group++){
   uint32_t sum=0;
   for(unsigned half=0;half<2;half++){
    uint16_t one[16],x[16];for(unsigned k=0;k<16;k++){one[k]=0x3f80;x[k]=input[group*32+half*16+k];}
    for(unsigned phase=0;phase<4;phase+=2)if(!PioneerBlackholeBf16Arithmetic::Instruction(one,x,phase,sum,sum))return false;
   }
   for(unsigned row=0;row<32;row++){
    const auto* p=original+row*144;unsigned scale,minimum;
    if(group<4){scale=p[4+group]&63;minimum=p[8+group]&63;}
    else{scale=(p[8+group]&15)|((p[group]>>6)<<4);minimum=(p[8+group]>>4)|((p[4+group]>>6)<<4);}
    uint32_t coefficient=0,offset=0;
    if(!PioneerQ38BinaryScale(uint16_t(p[0])|uint16_t(p[1])<<8,int(scale),coefficient)
     || !PioneerQ38BinaryScale(uint16_t(p[2])|uint16_t(p[3])<<8,int(minimum),offset))return false;
    offset^=0x80000000;uint32_t dot=0;
    for(unsigned half=0;half<2;half++){
     uint16_t q[16],x[16];
     for(unsigned k=0;k<16;k++){
      const unsigned column=half*16+k,code=(p[16+(group/2)*32+column]>>((group&1)*4))&15;
      q[k]=uint16_t(PioneerFmaBits(float(code))>>16);x[k]=input[group*32+column];
     }
     for(unsigned phase=0;phase<4;phase+=2)if(!PioneerBlackholeBf16Arithmetic::Instruction(q,x,phase,dot,dot))return false;
    }
    const uint32_t a=PioneerBlackholeFmaBits(coefficient,dot,0x80000000),b=PioneerBlackholeFmaBits(offset,sum,0x80000000);
    const uint32_t contribution=PioneerBlackholeFmaBits(0x3f800000,a,b);
    accumulator[row]=PioneerBlackholeFmaBits(0x3f800000,accumulator[row],contribution);
   }
  }return true;
 }
};
#endif
