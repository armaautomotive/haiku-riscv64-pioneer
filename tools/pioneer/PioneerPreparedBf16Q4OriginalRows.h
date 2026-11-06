/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PREPARED_BF16_Q4_ORIGINAL_ROWS_H
#define PIONEER_PREPARED_BF16_Q4_ORIGINAL_ROWS_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Original-row permutation is isolated diagnostic preparation ONLY
#endif
#include "PioneerNativeCompactQ4StreamData.h" // private authenticated104 bytes
#include <array>
// Caller MUST retain the original104 SAME-FD source owner in Ready state.
// That owner verifies all private bytes against original reads. This is ONLY
// the inverse lossless byte permutation,not another file open/FP product.
struct PioneerPreparedBf16Q4OriginalRows {
 using D=PioneerNativeCompactQ4StreamData;
 static bool Block(unsigned sample,unsigned block,std::array<uint8_t,4608>& rows,std::array<uint16_t,256>& input){
  if(sample>=9 || block>=D::Width(sample)/256)return false;
  std::array<uint8_t,4608> packed{};
  for(unsigned w=0;w<1152;w++){const uint32_t v=D::Word(sample,block*1152+w);for(unsigned i=0;i<4;i++)packed[w*4+i]=uint8_t(v>>(i*8));}
  for(unsigned row=0;row<32;row++){
   for(unsigned b=0;b<16;b++)rows[row*144+b]=packed[4096+(row/16)*256+b*16+row%16];
   for(unsigned b=0;b<128;b++)rows[row*144+16+b]=packed[(row/16)*2048+b*16+row%16];
  }
  const unsigned first=D::Width(sample)/256*1152+block*128;
  for(unsigned k=0;k<256;k++)input[k]=uint16_t(D::Word(sample,first+k/2)>>(k%2*16));
  return true;
 }
};
#endif
