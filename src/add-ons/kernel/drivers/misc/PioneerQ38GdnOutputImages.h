/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_OUTPUT_IMAGES_H
#define PIONEER_Q38_GDN_OUTPUT_IMAGES_H
#if !defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE) || PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE != 1
#error Exact separate SAME-owner output execution role required
#endif
#include "PioneerQ38GdnOutputRootMain.h"
#include "PioneerQ38GdnOutputRootPrefix.h"
#include "PioneerQ38GdnOutputRootGraph.h"
#include "PioneerQ38GdnOutputPeerMain.h"
#include "PioneerQ38GdnOutputPeerAffine.h"
#include "PioneerQ38GdnOutputPeerHelper.h"
#include "PioneerQ38ProjectionGraphFootprint.h"
// Complete exact code DATA within SAME original snapshot,not a new map,
// reset or image admission grant. Shared boot is never image authority.
static inline uint32_t PioneerQ38GdnOutputProgramWord(unsigned w,uint32_t a){
 if(!PioneerQ38ProjectionGraphProgramAddress(w,a))return UINT32_MAX;
 if(a==0x6000)return 0x0006c06f;
 if(!w){
  if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38GdnOutputRootPrefix))return PioneerQ38GdnOutputRootPrefix[(a-0x64000)/4];
  if(a>=0x68000 && a-0x68000<sizeof(PioneerQ38GdnOutputRootGraph))return PioneerQ38GdnOutputRootGraph[(a-0x68000)/4];
  if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38GdnOutputRootMain))return PioneerQ38GdnOutputRootMain[(a-0x72000)/4];
 }else{
  if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38GdnOutputPeerHelper))return PioneerQ38GdnOutputPeerHelper[(a-0x64000)/4];
  if(a>=0x70b00 && a-0x70b00<sizeof(PioneerQ38GdnOutputPeerAffine))return PioneerQ38GdnOutputPeerAffine[(a-0x70b00)/4];
  if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38GdnOutputPeerMain))return PioneerQ38GdnOutputPeerMain[(a-0x72000)/4];
 }
 return 0; // padding ONLY in already saved complete code footprint
}
static inline bool PioneerQ38GdnOutputExecutable(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 if(a==0x6000)return true;
 return w?((a>=0x64000 && a-0x64000<sizeof(PioneerQ38GdnOutputPeerHelper))
  || (a>=0x70b00 && a-0x70b00<sizeof(PioneerQ38GdnOutputPeerAffine)) || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38GdnOutputPeerMain)))
  :((a>=0x64000 && a-0x64000<sizeof(PioneerQ38GdnOutputRootPrefix))
  || (a>=0x68000 && a-0x68000<sizeof(PioneerQ38GdnOutputRootGraph)) || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38GdnOutputRootMain)));
}
static_assert(sizeof(PioneerQ38GdnOutputRootPrefix)<=16384 && sizeof(PioneerQ38GdnOutputRootGraph)<=24576
 && sizeof(PioneerQ38GdnOutputRootMain)<=8192 && sizeof(PioneerQ38GdnOutputPeerMain)<=8192
 && sizeof(PioneerQ38GdnOutputPeerAffine)<=5376 && sizeof(PioneerQ38GdnOutputPeerHelper)<=16384,"existing code limits unchanged");
#endif
