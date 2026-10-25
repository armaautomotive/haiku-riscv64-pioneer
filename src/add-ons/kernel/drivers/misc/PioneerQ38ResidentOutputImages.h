/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_IMAGES_H
#define PIONEER_Q38_RESIDENT_OUTPUT_IMAGES_H
#include "PioneerQ38ResidentOutputFootprint.h"
#include "PioneerQ38ResidentOutputRootBoot.h"
#include "PioneerQ38ResidentOutputRootMain.h"
#include "PioneerQ38ResidentOutputRootConv.h"
#include "PioneerQ38ResidentOutputRootMatrix.h"
#include "PioneerQ38ResidentOutputRootCohort.h"
#include "PioneerQ38ResidentOutputPeerBoot.h"
#include "PioneerQ38ResidentOutputPeerMain.h"
#include "PioneerQ38ResidentOutputPeerAffine.h"
#include "PioneerQ38ResidentOutputPeerFfn.h"
#include "PioneerQ38ResidentOutputPeerService.h"
#include "PioneerQ38ResidentOutputPeerGdn.h"
// Complete authenticated code DATA ONLY. Not included by any native owner,
// old code registry, Work/CPU reference, or Assistant. In particular this
// does NOT authorize writing the conservative snapshot's arbitrary gaps.
// A separate genuine joint owner still owes original snapshots, physical
// code/model/init/source/clock/host/exclusivity, unheld terminals, ALL holds,
// restore and END. UNKNOWN retains the entire context without later IO.
struct PioneerQ38ResidentOutputCodeImage {
 uint32_t address;const uint32_t* data;unsigned words;
};
static inline PioneerQ38ResidentOutputCodeImage PioneerQ38ResidentOutputImage(unsigned w,unsigned section){
 if(w>=120 || section>=PioneerQ38ResidentOutputCodeSpanCount(w))return {UINT32_MAX,nullptr,0};
 const auto s=PioneerQ38ResidentOutputCodeSpan(w,section);const uint32_t* data=nullptr;
 if(!w){
  switch(section){
   case 0:data=PioneerQ38ResidentOutputRootBoot;break;
   case 1:data=PioneerQ38ResidentOutputRootConv;break;
   case 2:data=PioneerQ38ResidentOutputRootMatrix;break;
   case 3:data=PioneerQ38ResidentOutputRootMain;break;
   case 4:data=PioneerQ38ResidentOutputRootCohort;break;
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
static inline unsigned PioneerQ38ResidentOutputCodeBase(unsigned w){
 return w>=120?UINT32_MAX:w?TT_Q38_RESIDENT_OUTPUT_ROOT_CODE_WORDS+(w-1)*TT_Q38_RESIDENT_OUTPUT_PEER_CODE_WORDS:0;
}
static inline uint32_t PioneerQ38ResidentOutputCodeAddress(unsigned w,unsigned ordinal){
 if(w>=120 || ordinal>=PioneerQ38ResidentOutputCodeWords(w))return UINT32_MAX;
 for(unsigned i=0;i<PioneerQ38ResidentOutputCodeSpanCount(w);i++){
  const auto s=PioneerQ38ResidentOutputCodeSpan(w,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;
 }
 return UINT32_MAX;
}
// Refusal leaves the caller's output untouched. No implicit zero padding,
// no executable-PC grant, no caller-supplied bytes/addresses to an installer.
static inline bool PioneerQ38ResidentOutputProgramWord(unsigned w,uint32_t a,uint32_t& output){
 if(w>=120 || (a&3))return false;
 for(unsigned i=0;i<PioneerQ38ResidentOutputCodeSpanCount(w);i++){
  const auto s=PioneerQ38ResidentOutputImage(w,i);
  if(a>=s.address && a-s.address<s.words*4){output=s.data[(a-s.address)/4];return true;}
 }
 return false;
}
static_assert(sizeof(PioneerQ38ResidentOutputRootBoot)==4 && sizeof(PioneerQ38ResidentOutputRootConv)==6472
 && sizeof(PioneerQ38ResidentOutputRootMatrix)==14136 && sizeof(PioneerQ38ResidentOutputRootMain)==92
 && sizeof(PioneerQ38ResidentOutputRootCohort)==50692,"ALL five exact ROOT sections,not old cohort");
static_assert(sizeof(PioneerQ38ResidentOutputPeerBoot)==4 && sizeof(PioneerQ38ResidentOutputPeerFfn)==16352
 && sizeof(PioneerQ38ResidentOutputPeerService)==10300 && sizeof(PioneerQ38ResidentOutputPeerAffine)==5360
 && sizeof(PioneerQ38ResidentOutputPeerMain)==7940 && sizeof(PioneerQ38ResidentOutputPeerGdn)==35396,"ALL six exact QOL1 peer sections,not old GDN");
static_assert(TT_Q38_RESIDENT_OUTPUT_CODE_WORDS==2259571 && TT_Q38_RESIDENT_OUTPUT_CODE_WORDS*4==9038284,"exact complete code DATA,not snapshot size or lease");
#endif
