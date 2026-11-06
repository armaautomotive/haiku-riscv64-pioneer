/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_BF16_Q4_REFERENCE_H
#define PIONEER_Q38_PREPARED_BF16_Q4_REFERENCE_H
#include "PioneerQ38FactoredQ4Reference.h" // CPU diagnostic role exclusions
// ORIGINAL GGML bytes only; no prepared codec/producer header or callbacks.
// This is a new separately rounded full-BF16 contract, NOT exact old-order
// equivalence or a prediction of whole-model answer quality/performance.
struct PioneerQ38PreparedBf16Q4Reference {
 static bool Group(const uint8_t* original,const uint16_t* input,unsigned group,uint32_t* acc){
  if(!original || !input || !acc || group>=8)return false;
  uint32_t sum=0;
  for(unsigned half=0;half<2;half++){
   uint16_t ones[16],x[16];for(unsigned k=0;k<16;k++){ones[k]=0x3f80;x[k]=input[group*32+half*16+k];}
   for(unsigned phase:{0U,2U})if(!PioneerBlackholeBf16Arithmetic::Instruction(ones,x,phase,sum,sum))return false;
  }
  for(unsigned row=0;row<32;row++){
   const auto* p=original+row*144;unsigned scale,minimum;
   if(group<4){scale=p[4+group]&63;minimum=p[8+group]&63;}
   else{scale=(p[8+group]&15)|((p[group]>>6)<<4);minimum=(p[8+group]>>4)|((p[4+group]>>6)<<4);}
   uint32_t coefficient=0,offset=0;
   if(!PioneerQ38BinaryScale(uint16_t(p[0])|uint16_t(p[1])<<8,int(scale),coefficient)
    || !PioneerQ38BinaryScale(uint16_t(p[2])|uint16_t(p[3])<<8,int(minimum),offset))return false;
   uint32_t dot=0;
   for(unsigned correction=0;correction<2;correction++)for(unsigned half=0;half<2;half++){
    uint16_t weights[16],x[16];
    for(unsigned k=0;k<16;k++){
     const unsigned column=half*16+k,q=(p[16+(group/2)*32+column]>>((group&1)*4))&15;
     const int value=correction?int(q==15):int(q==15?14:q)-7;
     weights[k]=uint16_t(PioneerFmaBits(float(value))>>16);x[k]=input[group*32+column];
    }
    for(unsigned phase:{0U,2U})if(!PioneerBlackholeBf16Arithmetic::Instruction(weights,x,phase,dot,dot))return false;
   }
   const uint32_t seven=PioneerBlackholeFmaBits(0x40e00000,sum,0x80000000);
   dot=PioneerBlackholeFmaBits(0x3f800000,dot,seven);
   const uint32_t a=PioneerBlackholeFmaBits(coefficient,dot,0x80000000);
   const uint32_t b=PioneerBlackholeFmaBits(offset^0x80000000,sum,0x80000000);
   acc[row]=PioneerBlackholeFmaBits(0x3f800000,acc[row],PioneerBlackholeFmaBits(0x3f800000,a,b));
  }
  return true;
 }
};
#endif
