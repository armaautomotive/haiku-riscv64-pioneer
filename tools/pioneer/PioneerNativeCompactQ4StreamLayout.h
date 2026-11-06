/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_COMPACT_Q4_STREAM_LAYOUT_H
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LAYOUT_H
#include <stdint.h>
#include "PioneerQ38Q4FpuCompactStreamLayout.h"
#ifdef PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE
#include "PioneerQ38PreparedBf16Q4StreamLayout.h"
#endif
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
#include "PioneerQ38Q4KernelTiming.h"
#endif
// Distinct fixed DATA scopes. Full L1 journal includes BOTH buffers/gaps;
// GDDR access is ONLY the selected exact weight+input domains,not2MiB.
struct PioneerNativeCompactQ4StreamLayout:PioneerQ38Q4FpuCompactStreamLayout {
#if defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE) || defined(PIONEER_NATIVE_FACTORED_Q4_TRACE) || defined(PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE)
 static constexpr unsigned CodeWords=4096,DataBegin=0x87ff0;
#else
 static constexpr unsigned CodeWords=2048,DataBegin=Raw-64;
#endif
 static constexpr unsigned CodeEnd=0x70000+CodeWords*4,DataEnd=0xa6400,DataWords=(DataEnd-DataBegin)/4;
 static constexpr unsigned L1Words=1+CodeWords+2048+8+DataWords+3;
#ifdef PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE
 static constexpr unsigned MaxDramWords=68*2688,TotalPackets=29,TotalFlits=176;
 static constexpr unsigned PreparedBytes(unsigned k){return Width(k)?(k/256)*5632:0;}
#else
 static constexpr unsigned MaxDramWords=68*1280,TotalPackets=2*Packets,TotalFlits=2*Flits;
 static constexpr unsigned PreparedBytes(unsigned){return 0;}
#endif
 static constexpr unsigned WeightBytes(unsigned k){return Width(k)?(k/256)*4608:0;}
 static constexpr unsigned InputBytes(unsigned k){return Width(k)?(k/256)*512:0;}
 static constexpr unsigned DramWords(unsigned k){return (WeightBytes(k)+PreparedBytes(k)+InputBytes(k))/4;}
 static unsigned Address(unsigned i){
  if(i>=L1Words)return UINT32_MAX;
  if(!i)return 0x6000;
  i--;
  if(i<CodeWords)return 0x70000+i*4;
  i-=CodeWords;
  if(i<2048)return 0x6e000+i*4;
  i-=2048;
  if(i<8)return Frame+i*4;
  i-=8;
  if(i<DataWords)return DataBegin+i*4;
  return 0x17ffc0+(i-DataWords)*4;
 }
 static bool Contains(unsigned a){return !(a&3) && (a==0x6000 || (a>=0x6e000 && a<CodeEnd)
  || (a>=Frame && a<Frame+32) || (a>=DataBegin && a<DataEnd) || (a>=0x17ffc0 && a<0x17ffcc));}
 static unsigned DramAddress(unsigned k,unsigned i){
  if(!Width(k) || i>=DramWords(k))return UINT32_MAX;
  if(i<WeightBytes(k)/4)return DramWeight+i*4;
#ifdef PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE
  if(i<(WeightBytes(k)+PreparedBytes(k))/4)return 0x04080000+(i-WeightBytes(k)/4)*4;
#endif
  return DramInput+(i-(WeightBytes(k)+PreparedBytes(k))/4)*4;
 }
 static bool DramContains(unsigned k,unsigned a){return Width(k) && !(a&3)
  && ((a>=DramWeight && a-DramWeight<WeightBytes(k)) || (a>=DramInput && a-DramInput<InputBytes(k))
#ifdef PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE
  || (a>=0x04080000 && a-0x04080000<PreparedBytes(k))
#endif
  );}
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
   at=InputIndex(local);if(at!=UINT32_MAX)return (WeightBytes(k)+PreparedBytes(k))/4+block*128+at/4;
  }return UINT32_MAX;
 }
 static bool Immutable(unsigned a){
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
  if(a>=PioneerQ38Q4KernelTiming::Address && a<PioneerQ38Q4KernelTiming::Address+PioneerQ38Q4KernelTiming::Words*4)return false;
#endif
  return Contains(a) && (a==0x6000 || (a>=0x70000 && a<CodeEnd)
  || (a>=Frame && a<Frame+32) || (a>=DataBegin && a<DataEnd && !(a>=Output && a<Output+256)));}
};
static_assert(PioneerNativeCompactQ4StreamLayout::L1Words*4==
#if defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE) || defined(PIONEER_NATIVE_FACTORED_Q4_TRACE) || defined(PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE)
 148544
#else
 107632
#endif
 && PioneerNativeCompactQ4StreamLayout::MaxDramWords*4==
#ifdef PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE
 731136
#else
 348160
#endif
 ,"exact compact dual-buffer journals");
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
static_assert(PioneerQ38Q4KernelTiming::Address==PioneerNativeCompactQ4StreamLayout::Output+256
 && PioneerQ38Q4KernelTiming::Words==16
 && PioneerQ38Q4KernelTiming::Address+64<PioneerNativeCompactQ4StreamLayout::DataEnd
 && PioneerQ38Q4KernelTiming::Address+64<0xa0000-64,"timing DATA within same journal and outside either producer slot");
#endif
#endif
