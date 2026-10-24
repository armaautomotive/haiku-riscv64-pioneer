/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_IMAGES_H
#define PIONEER_Q38_NEXT_GDN_OUTPUT_IMAGES_H
// OFFLINE complete image DATA only; no native selector or public launch.
// The future native62 consumer still requires genuine native61 ownership.
#include "PioneerQ38NextGdnOutputRootMain.h"
#include "PioneerQ38NextGdnOutputRootPrefix.h"
#include "PioneerQ38NextGdnOutputRootGraph.h"
#include "PioneerQ38NextGdnOutputPeerMain.h"
#include "PioneerQ38NextGdnOutputPeerAffine.h"
#include "PioneerQ38NextGdnOutputPeerHelper.h"
#include "PioneerQ38ProjectionGraphFootprint.h"
// Complete exact code DATA within SAME original snapshot,not a new map,
// reset or image admission grant. Shared boot is never image authority.
static inline uint32_t PioneerQ38NextGdnOutputProgramWord(unsigned w,uint32_t a){
 if(!PioneerQ38ProjectionGraphProgramAddress(w,a))return UINT32_MAX;
 if(a==0x6000)return 0x0006c06f;
 if(!w){
  if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38NextGdnOutputRootPrefix))return PioneerQ38NextGdnOutputRootPrefix[(a-0x64000)/4];
  if(a>=0x68000 && a-0x68000<sizeof(PioneerQ38NextGdnOutputRootGraph))return PioneerQ38NextGdnOutputRootGraph[(a-0x68000)/4];
  if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38NextGdnOutputRootMain))return PioneerQ38NextGdnOutputRootMain[(a-0x72000)/4];
 }else{
  if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38NextGdnOutputPeerHelper))return PioneerQ38NextGdnOutputPeerHelper[(a-0x64000)/4];
  if(a>=0x70b00 && a-0x70b00<sizeof(PioneerQ38NextGdnOutputPeerAffine))return PioneerQ38NextGdnOutputPeerAffine[(a-0x70b00)/4];
  if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38NextGdnOutputPeerMain))return PioneerQ38NextGdnOutputPeerMain[(a-0x72000)/4];
 }
 return 0; // padding ONLY in already saved complete code footprint
}
static inline bool PioneerQ38NextGdnOutputExecutable(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 if(a==0x6000)return true;
 return w?((a>=0x64000 && a-0x64000<sizeof(PioneerQ38NextGdnOutputPeerHelper))
  || (a>=0x70b00 && a-0x70b00<sizeof(PioneerQ38NextGdnOutputPeerAffine)) || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38NextGdnOutputPeerMain)))
  :((a>=0x64000 && a-0x64000<sizeof(PioneerQ38NextGdnOutputRootPrefix))
  || (a>=0x68000 && a-0x68000<sizeof(PioneerQ38NextGdnOutputRootGraph)) || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38NextGdnOutputRootMain)));
}
static_assert(sizeof(PioneerQ38NextGdnOutputRootPrefix)<=16384 && sizeof(PioneerQ38NextGdnOutputRootGraph)<=24576
 && sizeof(PioneerQ38NextGdnOutputRootMain)<=8192 && sizeof(PioneerQ38NextGdnOutputPeerMain)<=8192
 && sizeof(PioneerQ38NextGdnOutputPeerAffine)<=5376 && sizeof(PioneerQ38NextGdnOutputPeerHelper)<=16384,"existing code limits unchanged");
#endif
