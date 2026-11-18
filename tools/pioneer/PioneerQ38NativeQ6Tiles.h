/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_Q6_TILES_H
#define PIONEER_Q38_NATIVE_Q6_TILES_H
#include "PioneerQ38NativeQ8Data.h"

// Separate exact Q6_K -> native signed-magnitude INT8 tile. 16 output
// rows x 256 K, byte-major as on the existing direct SrcA path. The
// original row/panel scale and FP16 base bytes are copied, not converted.
// DATA only: card load/sign format, counter closure, float arithmetic
// order, and a distinct native upload/boot identity remain caller work.
namespace PioneerQ38NativeQ6Tiles {
enum : unsigned {
 RawBytes=3392,TileBytes=4416,RawMeta=3072,MetaBytes=288,
 PanelBase=320,PanelBytes=256,Panels=16,Rows=16,BlockK=256
};
static_assert(PanelBase+PanelBytes*Panels==TileBytes && !(TileBytes&63) && !(PanelBase&63),
 "aligned native tile and operand panels");

static constexpr unsigned LowByte(unsigned k){return (k/128)*64+((k/32)&1)*32+k%32;}
static constexpr unsigned LowShift(unsigned k){return ((k/64)&1)*4;}
static constexpr unsigned HighByte(unsigned k){return 128+(k/128)*32+k%32;}
static constexpr unsigned HighShift(unsigned k){return ((k/32)&3)*2;}

static inline bool ValidMetadata(const uint8_t* data){
 for(unsigned r=0;r<Rows;r++)if((data[272+r]&0x7c)==0x7c)return false; // HALF exponent 31
 return true;
}

static inline bool Encode(const uint8_t* source,unsigned sourceBytes,uint8_t* output,unsigned outputBytes){
 if(!source || !output || sourceBytes!=RawBytes || outputBytes!=TileBytes
  || !PioneerQ38NativeQ8Data::Separate(reinterpret_cast<uintptr_t>(source),RawBytes,
                                      reinterpret_cast<uintptr_t>(output),TileBytes)
  || !ValidMetadata(source+RawMeta))return false;
 for(unsigned i=RawMeta+MetaBytes;i<RawBytes;i++)if(source[i])return false;
 // Every packed six-bit code is in [-32,31], hence inherently encodable.
 for(unsigned i=0;i<PanelBase;i++)output[i]=i<MetaBytes?source[RawMeta+i]:0U;
 for(unsigned i=0;i<BlockK;i++){
  const auto* lo=source+LowByte(i)*Rows;const auto* hi=source+HighByte(i)*Rows;
  for(unsigned r=0;r<Rows;r++){
   const int q=int(((lo[r]>>LowShift(i))&15U)|(((hi[r]>>HighShift(i))&3U)<<4))-32;
   output[PanelBase+i*Rows+r]=q<0?uint8_t(128-q):uint8_t(q);
  }
 }
 return true;
}

// Independent storage round-trip and diagnostics only. Reject illegal
// signed-magnitude values before materializing any recovered bytes.
static inline bool Decode(const uint8_t* source,unsigned sourceBytes,uint8_t* output,unsigned outputBytes){
 if(!source || !output || sourceBytes!=TileBytes || outputBytes!=RawBytes
  || !PioneerQ38NativeQ8Data::Separate(reinterpret_cast<uintptr_t>(source),TileBytes,
                                      reinterpret_cast<uintptr_t>(output),RawBytes)
  || !ValidMetadata(source))return false;
 for(unsigned i=MetaBytes;i<PanelBase;i++)if(source[i])return false;
 for(unsigned i=PanelBase;i<TileBytes;i++){
  unsigned q=source[i];if((q<128 && q>31) || q==128 || q>160)return false;
 }
 for(unsigned i=0;i<RawBytes;i++)output[i]=i>=RawMeta && i<RawMeta+MetaBytes?source[i-RawMeta]:0U;
 for(unsigned i=0;i<BlockK;i++)for(unsigned r=0;r<Rows;r++){
  unsigned q=source[PanelBase+i*Rows+r];q=q<128?32+q:160-q;
  output[LowByte(i)*Rows+r]|=uint8_t((q&15)<<LowShift(i));
  output[HighByte(i)*Rows+r]|=uint8_t((q>>4)<<HighShift(i));
 }
 return true;
}

// Bounded geometry for ONE new tile per read. Keep every request under
// the current 8 KiB cap rather than silently overflowing or assuming a
// larger NoC transaction works. A later linked worker must measure the
// exchange of more read commands for removal of in-worker Q6 decoding.
struct TwinFetch {
 enum : unsigned { Buffer0=0x98000,Buffer1=0x9a000,Limit=0x9ba80,UploadFloor=0x04000000,UploadEnd=0xc4000000 };
 unsigned source,raw,nextSource,nextRaw;
 bool first,hasNext;
 static bool Describe(unsigned inputs,unsigned groups,unsigned group,unsigned block,unsigned base,TwinFetch& result){
  if((inputs!=5120 && inputs!=6144) || !groups || groups>16 || group>=groups
   || base<UploadFloor || base>=UploadEnd || (base&63))return false;
  unsigned blocks=inputs/BlockK;
  if(block>=blocks || groups*blocks*TileBytes>UploadEnd-base)return false;
  unsigned raw=block&1?Buffer1:Buffer0,source=base+(group*blocks+block)*TileBytes;
  const bool next=block+1<blocks;
  result={source,raw,next?source+TileBytes:0U,next?(raw==Buffer0?Buffer1:Buffer0):0U,block==0,next};
  return true;
 }
 static bool PanelAddress(unsigned raw,unsigned panel,unsigned& address){
  if((raw!=Buffer0 && raw!=Buffer1) || panel>=Panels)return false;
  address=raw+PanelBase+panel*PanelBytes;
  return true;
 }
};
static_assert(TwinFetch::Buffer0+TileBytes<=TwinFetch::Buffer1 && TwinFetch::Buffer1+TileBytes<=TwinFetch::Limit
 && TileBytes<=8192 && TwinFetch::Limit<0xa0000,"stays inside the original twin-buffer arena");
}
#endif
