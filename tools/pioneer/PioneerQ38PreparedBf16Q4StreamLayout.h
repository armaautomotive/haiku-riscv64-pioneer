/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_BF16_Q4_STREAM_LAYOUT_H
#define PIONEER_Q38_PREPARED_BF16_Q4_STREAM_LAYOUT_H
#include "PioneerQ38Q4FpuCompactStreamLayout.h"
// Isolated comparison DATA proposal, NOT a full-model plan or native owner.
// SAME existing cache primary bytes; two2304B tiles,then two512B dense masks.
// The original compressed raw comparison source remains separate/unmodified.
struct PioneerQ38PreparedBf16Q4StreamLayout {
 using Old=PioneerQ38Q4FpuCompactStreamLayout;using Read=Old::Read;
 static constexpr unsigned Raw=0x90000,Mask=0x8c000,Input=0x96000,BufferStride=0x10000;
 static constexpr unsigned Header=64,DramWeight=0x04080000,DramInput=Old::DramInput;
 static constexpr unsigned TileBytes=2304,PrimaryBytes=4608,MaskBytes=1024,BlockBytes=5632;
 static constexpr unsigned Packets=3,Flits=96,Slice=1,Slices=3;
 static constexpr bool Width(unsigned k){return Old::Width(k);}
 static bool Route(unsigned route){return Old::Route(route);}
 static bool Packet(unsigned k,unsigned block,unsigned buffer,unsigned packet,Read& out){
  if(!Width(k) || block>=k/256 || buffer>=2 || packet>=Packets)return false;
  const unsigned source=DramWeight+block*BlockBytes,offset=buffer*BufferStride;
  out=packet==0?Read{source,Raw+offset,PrimaryBytes}:packet==1?
   Read{source+PrimaryBytes,Mask+offset,MaskBytes}:Read{DramInput+block*512,Input+offset,512};return true;
 }
 static unsigned HeaderAddress(unsigned buffer,unsigned packet){Read p{};return Packet(5120,0,buffer,packet,p)?p.destination-Header:~0U;}
 static bool HeaderWord(unsigned a){
  if(a&3)return false;
  for(unsigned b=0;b<2;b++)for(unsigned p=0;p<Packets;p++)if(a>=HeaderAddress(b,p) && a<HeaderAddress(b,p)+Header)return true;
  return false;
 }
};
static_assert(PioneerQ38PreparedBf16Q4StreamLayout::DramWeight+68*5632<=PioneerQ38Q4FpuCompactStreamLayout::DramInput
 && PioneerQ38Q4FpuCompactStreamLayout::DramWeight+68*4608<=PioneerQ38PreparedBf16Q4StreamLayout::DramWeight,
 "isolated old/prepared/input proposals disjoint, NOT GDDR allocation authority");
#endif
