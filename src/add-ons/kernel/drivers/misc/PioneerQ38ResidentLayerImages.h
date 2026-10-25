/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_IMAGES_H
#define PIONEER_Q38_RESIDENT_LAYER_IMAGES_H
#include "PioneerQ38ResidentLayerFootprint.h"
#include "PioneerQ38ResidentLayerRootBoot.h"
#include "PioneerQ38ResidentLayerRootMain.h"
#include "PioneerQ38ResidentLayerRootConv.h"
#include "PioneerQ38ResidentLayerRootMatrix.h"
#include "PioneerQ38ResidentLayerRootCohort.h"
#include "PioneerQ38ResidentLayerRootNorm.h"
#include "PioneerQ38ResidentLayerRootFfn.h"
#include "PioneerQ38ResidentOutputPeerBoot.h"
#include "PioneerQ38ResidentOutputPeerMain.h"
#include "PioneerQ38ResidentOutputPeerAffine.h"
#include "PioneerQ38ResidentOutputPeerFfn.h"
#include "PioneerQ38ResidentOutputPeerService.h"
#include "PioneerQ38ResidentOutputPeerGdn.h"
// Exact complete code DATA ONLY. NONE of the old registries/native owners/
// Work entry points installs this. ALL7 new ROOT sections were authenticated
// alongside ALL6 exact unchanged peer sections;reusing those constants does
// NOT reuse an old code/model/source/clock/context/initialized/owner grant.
struct PioneerQ38ResidentLayerCodeImage {uint32_t address;const uint32_t* data;unsigned words;};
static inline PioneerQ38ResidentLayerCodeImage PioneerQ38ResidentLayerImage(unsigned w,unsigned section){
 if(w>=120 || section>=PioneerQ38ResidentLayerCodeSpanCount(w))return {UINT32_MAX,nullptr,0};
 const auto s=PioneerQ38ResidentLayerCodeSpan(w,section);const uint32_t* data=nullptr;
 if(!w){
  switch(section){
   case 0:data=PioneerQ38ResidentLayerRootBoot;break;
   case 1:data=PioneerQ38ResidentLayerRootConv;break;
   case 2:data=PioneerQ38ResidentLayerRootMatrix;break;
   case 3:data=PioneerQ38ResidentLayerRootMain;break;
   case 4:data=PioneerQ38ResidentLayerRootCohort;break;
   case 5:data=PioneerQ38ResidentLayerRootNorm;break;
   case 6:data=PioneerQ38ResidentLayerRootFfn;break;
  }
 }else{
  switch(section){
   case 0:data=PioneerQ38ResidentOutputPeerBoot;break;
   case 1:data=PioneerQ38ResidentOutputPeerFfn;break;
   case 2:data=PioneerQ38ResidentOutputPeerService;break;
   case 3:data=PioneerQ38ResidentOutputPeerAffine;break;
   case 4:data=PioneerQ38ResidentOutputPeerMain;break;
   case 5:data=PioneerQ38ResidentOutputPeerGdn;break;
  }
 }
 return {s.address,data,s.words};
}
static inline unsigned PioneerQ38ResidentLayerCodeBase(unsigned w){
 return w>=120?UINT32_MAX:w?TT_Q38_RESIDENT_LAYER_ROOT_CODE_WORDS+(w-1)*TT_Q38_RESIDENT_LAYER_PEER_CODE_WORDS:0;
}
static inline uint32_t PioneerQ38ResidentLayerCodeAddress(unsigned w,unsigned ordinal){
 if(w>=120 || ordinal>=PioneerQ38ResidentLayerCodeWords(w))return UINT32_MAX;
 for(unsigned i=0;i<PioneerQ38ResidentLayerCodeSpanCount(w);i++){
  const auto s=PioneerQ38ResidentLayerCodeSpan(w,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;
 }
 return UINT32_MAX;
}
static inline bool PioneerQ38ResidentLayerProgramWord(unsigned w,uint32_t a,uint32_t& output){
 if(w>=120 || (a&3))return false;
 for(unsigned i=0;i<PioneerQ38ResidentLayerCodeSpanCount(w);i++){
  const auto s=PioneerQ38ResidentLayerImage(w,i);
  if(a>=s.address && a-s.address<s.words*4){output=s.data[(a-s.address)/4];return true;}
 }
 return false; // Caller output untouched. NO implicit zero-filled gaps.
}
static_assert(sizeof(PioneerQ38ResidentLayerRootBoot)==4 && sizeof(PioneerQ38ResidentLayerRootConv)==6472
 && sizeof(PioneerQ38ResidentLayerRootMatrix)==14136 && sizeof(PioneerQ38ResidentLayerRootMain)==128
 && sizeof(PioneerQ38ResidentLayerRootCohort)==59288 && sizeof(PioneerQ38ResidentLayerRootNorm)==5404
 && sizeof(PioneerQ38ResidentLayerRootFfn)==30496,"ALL7 exact NEW ROOT sections,not old68 code or ownership");
static_assert(sizeof(PioneerQ38ResidentOutputPeerBoot)==4 && sizeof(PioneerQ38ResidentOutputPeerFfn)==16352
 && sizeof(PioneerQ38ResidentOutputPeerService)==10300 && sizeof(PioneerQ38ResidentOutputPeerAffine)==5360
 && sizeof(PioneerQ38ResidentOutputPeerMain)==7940 && sizeof(PioneerQ38ResidentOutputPeerGdn)==35396,"SAME6 exact authenticated peer DATA sections");
#endif
