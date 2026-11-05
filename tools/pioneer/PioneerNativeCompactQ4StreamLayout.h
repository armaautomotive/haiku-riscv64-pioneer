/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_COMPACT_Q4_STREAM_LAYOUT_H
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LAYOUT_H
#include <stdint.h>
#include "PioneerQ38Q4FpuCompactStreamLayout.h"
// Distinct fixed DATA scopes. Full L1 journal includes BOTH buffers/gaps;
// GDDR access is ONLY the selected exact weight+input domains,not2MiB.
struct PioneerNativeCompactQ4StreamLayout:PioneerQ38Q4FpuCompactStreamLayout {
 static constexpr unsigned DataBegin=Raw-64,DataEnd=0xa6400,DataWords=(DataEnd-DataBegin)/4;
 static constexpr unsigned L1Words=1+2048+2048+8+DataWords+3,MaxDramWords=68*1280;
 static constexpr unsigned WeightBytes(unsigned k){return Width(k)?(k/256)*4608:0;}
 static constexpr unsigned InputBytes(unsigned k){return Width(k)?(k/256)*512:0;}
 static constexpr unsigned DramWords(unsigned k){return (WeightBytes(k)+InputBytes(k))/4;}
 static unsigned Address(unsigned i){
  if(i>=L1Words)return UINT32_MAX;
  if(!i)return 0x6000;
  i--;
  if(i<2048)return 0x70000+i*4;
  i-=2048;
  if(i<2048)return 0x6e000+i*4;
  i-=2048;
  if(i<8)return Frame+i*4;
  i-=8;
  if(i<DataWords)return DataBegin+i*4;
  return 0x17ffc0+(i-DataWords)*4;
 }
 static bool Contains(unsigned a){return !(a&3) && (a==0x6000 || (a>=0x6e000 && a<0x72000)
  || (a>=Frame && a<Frame+32) || (a>=DataBegin && a<DataEnd) || (a>=0x17ffc0 && a<0x17ffcc));}
 static unsigned DramAddress(unsigned k,unsigned i){
  if(!Width(k) || i>=DramWords(k))return UINT32_MAX;
  return i<WeightBytes(k)/4?DramWeight+i*4:DramInput+(i-WeightBytes(k)/4)*4;
 }
 static bool DramContains(unsigned k,unsigned a){return Width(k) && !(a&3)
  && ((a>=DramWeight && a-DramWeight<WeightBytes(k)) || (a>=DramInput && a-DramInput<InputBytes(k)));}
 static unsigned PackedIndex(unsigned a){
  if(a>=Raw-64 && a<Raw-64+16*RawStride){
   const unsigned rel=a-(Raw-64),slot=rel/RawStride,within=rel%RawStride;
   if(within<64)return UINT32_MAX;
   return (slot%4/2)*2048+(slot/4)*512+(slot%2)*256+within-64;
  }
  if(a>=Metadata-64 && a<Metadata-64+2*MetadataStride){
   const unsigned rel=a-(Metadata-64),within=rel%MetadataStride;
   return within<64?UINT32_MAX:4096+(rel/MetadataStride)*256+within-64;
  }
  return UINT32_MAX;
 }
 static unsigned InputIndex(unsigned a){
  if(a<Input-64 || a>=Input-64+8*InputStride)return UINT32_MAX;
  const unsigned rel=a-(Input-64),within=rel%InputStride;
  return within<64?UINT32_MAX:(rel/InputStride)*64+within-64;
 }
 static unsigned FinalPayloadIndex(unsigned k,unsigned a){
  if(!Width(k) || (a&3))return UINT32_MAX;
  for(unsigned b=0;b<2;b++){
   const unsigned local=a-b*BufferStride,block=k/256-2+b;
   unsigned at=PackedIndex(local);if(at!=UINT32_MAX)return block*1152+at/4;
   at=InputIndex(local);if(at!=UINT32_MAX)return WeightBytes(k)/4+block*128+at/4;
  }return UINT32_MAX;
 }
 static bool Immutable(unsigned a){return Contains(a) && (a==0x6000 || (a>=0x70000 && a<0x72000)
  || (a>=Frame && a<Frame+32) || (a>=DataBegin && a<DataEnd && !(a>=Output && a<Output+256)));}
};
static_assert(PioneerNativeCompactQ4StreamLayout::L1Words*4==107632 && PioneerNativeCompactQ4StreamLayout::MaxDramWords*4==348160,"exact compact dual-buffer journals");
#endif
