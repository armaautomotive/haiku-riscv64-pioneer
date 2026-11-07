/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_BFP4_PROMPT_QUALITY_H
#define PIONEER_BFP4_PROMPT_QUALITY_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error BFP4 prompt quality arithmetic is a separate offline diagnostic, never Work or device
#endif
#include "PioneerBlackholeBlockFloat.h"
#include "PioneerBlackholeBf16Arithmetic.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>
// This is a CPU observer of one converted band, not an accelerator engine.
// Caller authenticates source/sample/input and supplies original-Q4 results.
namespace PioneerBfp4PromptQuality {
struct Result {double relativeL2,maxAbsolute;};
inline bool Compute(const uint8_t* packed,size_t packedBytes,const uint32_t* input,const uint32_t* original,Result& result){
 if(!packed || packedBytes!=160*576 || !input || !original)return false;
 std::array<uint16_t,5120> operands={};
 for(unsigned i=0;i<5120;i++){
  uint32_t bits=input[i];if((bits&0x7f800000)==0x7f800000)return false;
  if((bits&0x7f800000)==0)operands[i]=uint16_t((bits>>16)&0x8000);
  else {bits+=0x7fff+((bits>>16)&1);operands[i]=uint16_t(bits>>16);if((operands[i]&0x7f80)==0x7f80)return false;}
 }
 std::array<uint32_t,32> actual={};
 for(unsigned tile=0;tile<160;tile++){
  std::array<float,1024> decoded={};
  if(!PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,packed+tile*576,576,decoded.data(),1024))return false;
  for(unsigned row=0;row<32;row++)for(unsigned half=0;half<2;half++){
   uint16_t a[16],b[16];
   for(unsigned k=0;k<16;k++){
    uint32_t bits=0;std::memcpy(&bits,&decoded[row*32+half*16+k],4);if(bits&65535)return false;
    a[k]=uint16_t(bits>>16);b[k]=operands[tile*32+half*16+k];
   }
   for(unsigned phase:{0U,2U})if(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,actual[row],actual[row]))return false;
  }
 }
 long double energy=0,error=0;double maximum=0;
 for(unsigned i=0;i<32;i++){
  float reference=0,observed=0;std::memcpy(&reference,original+i,4);std::memcpy(&observed,actual.data()+i,4);
  if(!std::isfinite(reference) || !std::isfinite(observed))return false;
  const long double d=static_cast<long double>(observed)-reference;energy+=static_cast<long double>(reference)*reference;error+=d*d;
  maximum=std::max(maximum,double(std::fabs(d)));
 }
 if(!(energy>0))return false;
 Result next={double(std::sqrt(error/energy)),maximum};if(!std::isfinite(next.relativeL2) || !std::isfinite(next.maxAbsolute))return false;
 result=next;return true;
}
}
#endif
