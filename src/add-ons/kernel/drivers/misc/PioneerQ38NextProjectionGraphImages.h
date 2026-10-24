/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_PROJECTION_GRAPH_IMAGES_H
#define PIONEER_Q38_NEXT_PROJECTION_GRAPH_IMAGES_H
#include "PioneerQ38NextProjectionGraphRootMain.h"
#include "PioneerQ38NextProjectionGraphRootPrefix.h"
#include "PioneerQ38NextProjectionGraphRootGraph.h"
#include "PioneerQ38ProjectionGraphPeerMain.h"
#include "PioneerQ38ProjectionGraphPeerAffine.h"
#include "PioneerQ38ProjectionGraphPeerHelper.h"
// Distinct immutable QPS2 ROOT + EXACT unchanged QFP3 peers,DATA ONLY.
// No selector/FD/copy/launch/producer grant. A future SAME native decoder
// must claim its genuine NextLayerInput,all original learned pages,and ALL
// existing snapshots/source/host/clock/leases BEFORE replacing any code.
// Full snapshot extent is inherited from that actual decoder,not this image.
static inline bool PioneerQ38NextProjectionGraphProgramAddress(unsigned w,uint32_t a){
 return w<120 && !(a&3) && (a==0x6000 || (a>=0x64000 && a-0x64000<(w?16384U:40960U))
  || (a>=0x70000 && a<0x74000));
}
static inline uint32_t PioneerQ38NextProjectionGraphProgramWord(unsigned w,uint32_t a){
 if(!PioneerQ38NextProjectionGraphProgramAddress(w,a))return UINT32_MAX;
 if(a==0x6000)return 0x0006c06f;
 if(!w){
  if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38NextProjectionGraphRootPrefix))return PioneerQ38NextProjectionGraphRootPrefix[(a-0x64000)/4];
  if(a>=0x68000 && a-0x68000<sizeof(PioneerQ38NextProjectionGraphRootGraph))return PioneerQ38NextProjectionGraphRootGraph[(a-0x68000)/4];
  if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38NextProjectionGraphRootMain))return PioneerQ38NextProjectionGraphRootMain[(a-0x72000)/4];
 }else{
  if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38ProjectionGraphPeerHelper))return PioneerQ38ProjectionGraphPeerHelper[(a-0x64000)/4];
  if(a>=0x70b00 && a-0x70b00<sizeof(PioneerQ38ProjectionGraphPeerAffine))return PioneerQ38ProjectionGraphPeerAffine[(a-0x70b00)/4];
  if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38ProjectionGraphPeerMain))return PioneerQ38ProjectionGraphPeerMain[(a-0x72000)/4];
 }
 return 0; // Padding inside separately retained program windows ONLY.
}
static_assert(sizeof(PioneerQ38NextProjectionGraphRootMain)<=8192
 && sizeof(PioneerQ38NextProjectionGraphRootPrefix)<=16384
 && sizeof(PioneerQ38NextProjectionGraphRootGraph)<=24576,"exact old windows;NO stack/mapping expansion");
static_assert(sizeof(PioneerQ38ProjectionGraphPeerMain)==8164 && sizeof(PioneerQ38ProjectionGraphPeerAffine)==5248
 && sizeof(PioneerQ38ProjectionGraphPeerHelper)==16352,"unchanged whole QFP3 peers,not retagged helper subsets");
#endif
