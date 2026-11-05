/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_COMPACT_DATA_H
#define PIONEER_Q38_Q4_FPU_COMPACT_DATA_H
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Compact Q4 FPU storage is separate offboard data,not a driver or Work source
#endif
#include <stdint.h>
// Lossless layout ONLY:no coefficient multiplication or BF16 conversion.
// First4096 bytes are the existing FPU quant layout. Last512 bytes are TWO
// original byte-major256B metadata prefixes,one per16 output rows.
struct PioneerQ38Q4FpuCompactData {
 enum : unsigned {Version=1,OriginalBytes=4608,Bytes=4608,QuantBytes=4096,MetadataBytes=512};
 static bool Prepare(const uint8_t* source,unsigned count,uint8_t* output,unsigned capacity){
  if(!source || !output || count!=OriginalBytes || capacity<Bytes)return false;
  const uintptr_t a=reinterpret_cast<uintptr_t>(source),b=reinterpret_cast<uintptr_t>(output),limit=~uintptr_t(0);
  if(a>limit-count || b>limit-Bytes || !(a+count<=b || b+Bytes<=a))return false;
  for(unsigned row=0;row<32;row++)for(unsigned high=1;high<=3;high+=2)
   if((source[row*144+high]&0x7c)==0x7c)return false;
  // All refusal checks precede the first destination write.
  for(unsigned row=0;row<32;row++){
   const auto* q=source+row*144;
   for(unsigned byte=0;byte<128;byte++)output[(row/16)*2048+byte*16+row%16]=q[16+byte];
   for(unsigned byte=0;byte<16;byte++)output[QuantBytes+(row/16)*256+byte*16+row%16]=q[byte];
  }
  return true;
 }
};
#endif
