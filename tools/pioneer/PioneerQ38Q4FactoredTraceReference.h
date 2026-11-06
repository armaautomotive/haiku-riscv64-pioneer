/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Original-byte TRACE prediction is separate CPU diagnostic ONLY
#endif
#ifndef PIONEER_Q38_Q4_FACTORED_TRACE_REFERENCE_H
#define PIONEER_Q38_Q4_FACTORED_TRACE_REFERENCE_H
#include "PioneerBlackholeBf16Arithmetic.h"
#include "PioneerBlackholeFma.h"
#include "PioneerQ38BinaryScale.h"
// Original bytes -> independent predicted observations. No producer, device,
// supplied dot/sum, inference context or CPU fallback. Hashes are diagnostics;
// the final32 FP32 outputs remain exact required comparisons.
class PioneerQ38Q4FactoredTraceReference {
 uint32_t accumulator[32]={},blocks[20]={},stages[4]={2166136261U,2166136261U,2166136261U,2166136261U},first[8]={};
 unsigned completed=0;uint32_t blockHash=2166136261U;
 static uint32_t Mix(uint32_t h,uint32_t v){h+=v;h+=h<<10;return h^(h>>6);}
 void Record(unsigned stage,uint32_t v){stages[stage]=Mix(stages[stage],v);blockHash=Mix(Mix(blockHash,stage),v);}
 static unsigned Code(const uint8_t* raw,unsigned row,unsigned group,unsigned k){return (raw[row*144+16+(group/2)*32+k]>>((group&1)*4))&15;}
 static uint32_t PackedWord(const uint8_t* raw,unsigned group,unsigned word){
  uint32_t value=0;
  for(unsigned b=0;b<4;b++){
   const unsigned i=word*4+b,face=i/256,k=(face%2)*16+(i%256)/16,row=(face/2)*16+i%16;
   value|=uint32_t(Code(raw,row,group,k)*8)<<(b*8);
  }return value;
 }
public:
 bool Block(const uint8_t* raw,const uint16_t* input,unsigned block){
  if(!raw || !input || block>=20 || block!=completed)return false;
  for(unsigned group=0;group<8;group++){
   for(unsigned i=0;i<4;i++)Record(0,0);
   for(unsigned i=0;i<16;i++)Record(0,0x82828282);
   for(unsigned i=0;i<256;i++)Record(0,PackedWord(raw,group,i));
   const bool capture=!block && !group;
   if(capture){first[0]=PackedWord(raw,group,0);first[1]=PackedWord(raw,group,4);}
   uint32_t sum=0,dots[32],coefficients[32],minimums[32];
   for(unsigned half=0;half<2;half++){
    uint16_t a[16],b[16];for(unsigned k=0;k<16;k++){a[k]=0x3f80;b[k]=input[group*32+half*16+k];}
    for(unsigned phase=0;phase<4;phase+=2)if(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,sum,sum))return false;
   }
   Record(1,sum);if(capture)first[2]=sum;
   for(unsigned row=0;row<32;row++){
    uint32_t dot=0;
    for(unsigned half=0;half<2;half++){
     uint16_t a[16],b[16];for(unsigned k=0;k<16;k++){a[k]=uint16_t(PioneerFmaBits(float(Code(raw,row,group,half*16+k)))>>16);b[k]=input[group*32+half*16+k];}
     for(unsigned phase=0;phase<4;phase+=2)if(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,dot,dot))return false;
    }
    dots[row]=dot;Record(1,dot);if(capture && !row)first[3]=dot;
   }
   for(unsigned row=0;row<32;row++){
    const auto* p=raw+row*144;unsigned scale,minimum;
    if(group<4){scale=p[4+group]&63;minimum=p[8+group]&63;}
    else{scale=(p[8+group]&15)|((p[group]>>6)<<4);minimum=(p[8+group]>>4)|((p[4+group]>>6)<<4);}
    if(!PioneerQ38BinaryScale(uint16_t(p[0])|uint16_t(p[1])<<8,int(scale),coefficients[row])
     || !PioneerQ38BinaryScale(uint16_t(p[2])|uint16_t(p[3])<<8,int(minimum),minimums[row]))return false;
    minimums[row]^=0x80000000;Record(2,coefficients[row]);Record(2,minimums[row]);
    if(capture && !row){first[4]=coefficients[row];first[5]=minimums[row];first[6]=accumulator[row];}
   }
   for(unsigned row=0;row<32;row++){
    uint32_t a=PioneerBlackholeFmaBits(coefficients[row],dots[row],0x80000000),b=PioneerBlackholeFmaBits(minimums[row],sum,0x80000000);
    accumulator[row]=PioneerBlackholeFmaBits(0x3f800000,accumulator[row],PioneerBlackholeFmaBits(0x3f800000,a,b));
    Record(3,accumulator[row]);if(capture && !row)first[7]=accumulator[row];
   }
  }
  blocks[completed++]=blockHash;blockHash=2166136261U;return true;
 }
 bool Ready()const{return completed==20;}
 bool Export(uint32_t* output)const{
  if(!output || !Ready())return false;
  for(unsigned i=0;i<32;i++)output[i]=accumulator[i];
  for(unsigned i=0;i<20;i++)output[32+i]=blocks[i];
  for(unsigned i=0;i<4;i++)output[52+i]=stages[i];
  for(unsigned i=0;i<8;i++)output[56+i]=first[i];
  return true;
 }
};
#endif
