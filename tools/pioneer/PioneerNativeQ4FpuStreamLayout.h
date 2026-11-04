/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_Q4_FPU_STREAM_LAYOUT_H
#define PIONEER_NATIVE_Q4_FPU_STREAM_LAYOUT_H
#include <stdint.h>
#include "PioneerQ38Q4FpuStreamLayout.h"
#include "PioneerNativeQ4FpuLayout.h"
// DATA geometry only. Full spans include BOTH producer buffers and all gaps.
// One selected exact GDDR range is journaled;never grant a full2MiB write span.
struct PioneerNativeQ4FpuStreamLayout:PioneerQ38Q4FpuStreamLayout {
 static constexpr unsigned DataBegin=Raw-64,DataEnd=0xa6400,DataWords=(DataEnd-DataBegin)/4;
 static constexpr unsigned L1Words=1+2048+2048+8+DataWords+3,MaxDramWords=68*1664;
 static constexpr unsigned WeightBytes(unsigned k){return Width(k)?(k/256)*6144:0;}
 static constexpr unsigned InputBytes(unsigned k){return Width(k)?(k/256)*512:0;}
 static constexpr unsigned DramWords(unsigned k){return (WeightBytes(k)+InputBytes(k))/4;}
 static unsigned Address(unsigned i) {
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
 static bool Contains(unsigned a) {
  return !(a&3) && (a==0x6000 || (a>=0x6e000 && a<0x72000) || (a>=Frame && a<Frame+32)
   || (a>=DataBegin && a<DataEnd) || (a>=0x17ffc0 && a<0x17ffcc));
 }
 static unsigned DramAddress(unsigned k,unsigned i) {
  if(!Width(k) || i>=DramWords(k))return UINT32_MAX;
  return i<WeightBytes(k)/4?DramWeight+i*4:DramInput+(i-WeightBytes(k)/4)*4;
 }
 static bool DramContains(unsigned k,unsigned a) {
  return Width(k) && !(a&3) && ((a>=DramWeight && a-DramWeight<WeightBytes(k))
   || (a>=DramInput && a-DramInput<InputBytes(k)));
 }
 // Final producer slots contain the last even/odd K256 blocks,from job2.
 // Dense data index,not a native address or host upload capability.
 static unsigned FinalPayloadIndex(unsigned k,unsigned a) {
  if(!Width(k) || (a&3))return UINT32_MAX;
  for(unsigned b=0;b<2;b++) {
   unsigned local=a-b*BufferStride,block=k/256-2+b;
   unsigned at=PioneerNativeQ4FpuLayout::PreparedIndex(local);
   if(at!=UINT32_MAX)return block*1536+at/4;
   at=PioneerNativeQ4FpuLayout::InputIndex(local);
   if(at!=UINT32_MAX)return WeightBytes(k)/4+block*128+at/4;
  }
  return UINT32_MAX;
 }
 static bool Immutable(unsigned a) {
  return Contains(a) && (a==0x6000 || (a>=0x70000 && a<0x72000) || (a>=Frame && a<Frame+32)
   || (a>=DataBegin && a<DataEnd && !(a>=Output && a<Output+256)));
 }
};
static_assert(PioneerNativeQ4FpuStreamLayout::L1Words*4==107632 &&
 PioneerNativeQ4FpuStreamLayout::MaxDramWords*4==452608,"bounded full dual-buffer L1/GDDR heap journals");
#endif
