/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_RECURRENT_COHORT_PLAN_H
#define PIONEER_Q38_MIXED_RECURRENT_COHORT_PLAN_H
#include "PioneerQ38MixedRecurrentCohortLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputPlan.h"
// Immutable canonical HEAP DATA only. Reuse numeric table construction,
// NOT old external jobs/sequence hints,images,source or native authority.
class PioneerQ38MixedRecurrentCohortPlan {
 PioneerQ38ResidentOutputPlan numeric;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 uint32_t header[64]={},conv[PioneerQ38MixedRecurrentCohortLayout::ConvWords]={},gdn[PioneerQ38MixedRecurrentCohortLayout::GdnWords]={};
 unsigned requestNumber=0;bool attempted=false,ready=false;
public:
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned layer,unsigned position,unsigned bank,unsigned slot,unsigned token,
  unsigned phase,unsigned request,const PioneerQ38GdnIdentity& id,const uint32_t* sources){
  if(attempted)return false;
  attempted=true;if(!request || request>=0x7fffffff)return false;
  uint32_t zero[119]={};
  if(!numeric.Build(c,layer,position,bank,slot,token,phase,id,sources,zero))return false;
  namespace L=PioneerQ38MixedRecurrentCohortLayout;
  const auto* m=numeric.Manifest();canonical=&c;requestNumber=request;
  for(unsigned i=0;i<40;i++)header[i]=m[i];
  header[0]=L::Magic;header[2]=64;header[15]=token;
  const unsigned fields[]={L::Conv,L::ConvShadow,L::Gdn,L::GdnShadow,L::ConvWords,L::GdnWords,L::PeerBoots,L::PeerJobs,
   request,L::Nonces,L::Counters,1};
  for(unsigned i=0;i<12;i++)header[40+i]=fields[i];
  for(unsigned i=0;i<L::ConvWords;i++)conv[i]=m[TT_Q38_COHORT_CONV_WORD+i];
  for(unsigned i=0;i<L::GdnWords;i++)gdn[i]=m[TT_Q38_COHORT_GDN_WORD+i];
  ready=true;if(Matches(c))return true;
  ready=false;return false;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  namespace L=PioneerQ38MixedRecurrentCohortLayout;
  if(!ready || canonical!=&c || !L::Shape(header) || header[48]!=requestNumber)return false;
  const auto* m=numeric.Manifest();if(!m)return false;
  const PioneerQ38GdnIdentity id={uint64_t(header[32])|(uint64_t(header[33])<<32),uint64_t(header[34])|(uint64_t(header[35])<<32),
   uint64_t(header[36])|(uint64_t(header[37])<<32),uint64_t(header[38])|(uint64_t(header[39])<<32)};
  uint32_t zero[119]={};
  if(!numeric.Matches(c,header[3],header[4],header[7],header[14],header[15],header[5],id,m+TT_Q38_COHORT_SOURCE_WORD,zero))return false;
  for(unsigned i=1;i<40;i++)if(i!=2 && i!=15 && header[i]!=m[i])return false;
  for(unsigned i=0;i<L::ConvWords;i++)if(conv[i]!=m[TT_Q38_COHORT_CONV_WORD+i])return false;
  for(unsigned i=0;i<L::GdnWords;i++)if(gdn[i]!=m[TT_Q38_COHORT_GDN_WORD+i])return false;
  for(unsigned w=0;w<120;w++){
   if(!L::ConvShape(header,conv+w*384,w))return false;
   if(w && w<=96 && !L::GdnShape(header,gdn+(w-1)*192,conv+w*384,w))return false;
  }
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  return canonical && Matches(*canonical) && numeric.LearnedPagesPresent(pages,count);
 } // includes numeric output projection pages too;NOT initialized state/input producer or source authority
 const uint32_t* Header()const{return ready?header:nullptr;}
 const uint32_t* Conv()const{return ready?conv:nullptr;}
 const uint32_t* Gdn()const{return ready?gdn:nullptr;}
};
static_assert(sizeof(PioneerQ38MixedRecurrentCohortPlan)>786432 && sizeof(PioneerQ38MixedRecurrentCohortPlan)<1572864,"complete canonical cohort tables HEAP ONLY");
#endif
