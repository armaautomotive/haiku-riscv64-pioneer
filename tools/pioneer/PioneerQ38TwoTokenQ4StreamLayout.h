/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWO_TOKEN_Q4_STREAM_LAYOUT_H
#define PIONEER_Q38_TWO_TOKEN_Q4_STREAM_LAYOUT_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
// Isolated returning Q4 geometry, not a live allocation or ownership grant.
namespace PioneerQ38TwoTokenQ4StreamLayout {
enum : unsigned {Magic=0x33543851,Fault=0x46543853,VectorMagic=0x34543851,VectorFault=0x46543856,Descriptor=0x74000,
 Tile=0x90000,TileBytes=2304,Panel=0x91000,Input0=0xa0000,Input1=0xb0000,VectorInput1=0x97000,
 Scale0=0x94000,Scale1=0x94400,Output=0x84000,WidthMax=17408,
 FirstAddress=0x04000000,EndAddress=0xc4000000,Self=129,Words=16,
 SavedBase=0x6e000,SavedEnd=0xb0000};
static_assert(Tile+TileBytes<=Panel && Panel+640<Scale0 && Scale0+68*4<=Scale1
 && Scale1+68*4<=Input0 && Input0+WidthMax*2<=Input1 && Input1+WidthMax*2<0x17ffc0,
 "two disjoint full input+scale vectors, raw scratch, panel and terminal");
static_assert(Scale1+68*4<=VectorInput1 && VectorInput1+WidthMax*2<=Input0
 && Input0+WidthMax*2<=SavedEnd && Descriptor>=SavedBase,
 "Q8T4 keeps both max-width inputs inside the already saved diagnostic window");
template<unsigned Expected=Magic> static inline bool Shape(const uint32_t* job){
 static_assert(Expected==Magic || Expected==VectorMagic,"only the separate two-row stream roles");
 const unsigned width=job[5],first=job[6];
 if(job[0]!=Expected || job[1]!=12 || job[2]!=TileBytes || job[3] || job[4]
  || (width!=5120 && width!=6144 && width!=WidthMax)
  || first<FirstAddress || first>=EndAddress || (first&63)
  || (width/256)*TileBytes>EndAddress-first || job[7]>=7)return false;
 for(unsigned w=8;w<Words;w++)if(job[w])return false;
 return true;
}
static inline unsigned Input(unsigned t){return t?Input1:Input0;}
static inline unsigned VectorInput(unsigned t){return t?VectorInput1:Input0;}
static inline unsigned Scale(unsigned t){return t?Scale1:Scale0;}
static inline unsigned Counter(unsigned i){
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
 static constexpr unsigned at[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return at[i];
#else
 const unsigned at[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return at[i];
#endif
}
static inline unsigned Delta(unsigned i){return i==1?TileBytes/64:unsigned(i==0 || i==2 || i==3 || i==6);}
}
#endif
