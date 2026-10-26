/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_LINK_PLAN_H
#define PIONEER_Q38_RESIDENT_LAYER_LINK_PLAN_H
#include "PioneerQ38ResidentLayerPlan.h"
#include "PioneerQ38LearnedRmsPlan.h"
#include "PioneerQ38ResidentLayerLinkShape.h"
// Canonical immutable DATA ONLY. Keeping pointers to the genuine recipe
// rejects same-shaped foreign plans; it does not mint runtime lineage.
class PioneerQ38ResidentLayerLinkPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38ResidentLayerPlan* parent=nullptr;
 uint32_t descriptor[TT_Q38_LAYER_LINK_WORDS]={};bool attempted=false,ready=false;
 static bool Describe(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38ResidentLayerPlan& p,uint32_t* d){
  if(!c.Ready() || !p.CanonicalRecipe())return false;
  const auto* l=p.LayerManifest();if(!l || l[3]>=63 || l[7]>=0x7ffffff8)return false;
  const PioneerQ38GdnIdentity id={uint64_t(l[32])|(uint64_t(l[33])<<32),uint64_t(l[34])|(uint64_t(l[35])<<32),
   uint64_t(l[36])|(uint64_t(l[37])<<32),uint64_t(l[38])|(uint64_t(l[39])<<32)};
  const auto* m=p.Manifest();
  if(!p.Matches(c,l[3],l[4],l[8],m[14],l[11],l[5],id,m+TT_Q38_COHORT_SOURCE_WORD,m+TT_Q38_COHORT_SEQUENCE_WORD))return false;
  PioneerQ38LearnedRmsPlan norm;
  if(!norm.Build(c,l[3]+1,0,l[4],l[8],id,l+16,l[7]+6))return false;
  for(unsigned i=0;i<TT_Q38_LAYER_LINK_WORDS;i++)d[i]=0;
  d[0]=TT_Q38_LAYER_LINK_MARKER;d[1]=1;d[2]=TT_Q38_LAYER_LINK_WORDS;
  d[3]=l[3];d[4]=l[3]+1;d[5]=l[4];d[6]=l[11];d[7]=l[8];d[8]=l[9];d[9]=l[7]+5;d[10]=l[7]+6;
  d[11]=5120;d[12]=TT_Q38_RESIDENT_LAYER_RECEIPT_MARKER;d[13]=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER;d[14]=d[15]=4;
  for(unsigned i=16;i<40;i++)d[i]=l[i];
  for(unsigned i=0;i<64;i++)d[64+i]=norm.Descriptor()[i];
  return PioneerQ38ResidentLayerLinkShape(d,l);
 }
public:
 PioneerQ38ResidentLayerLinkPlan()=default;
 PioneerQ38ResidentLayerLinkPlan(const PioneerQ38ResidentLayerLinkPlan&)=delete;
 PioneerQ38ResidentLayerLinkPlan& operator=(const PioneerQ38ResidentLayerLinkPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38ResidentLayerPlan& p){
  if(attempted)return false;attempted=true;canonical=&c;parent=&p;ready=Describe(c,p,descriptor);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38ResidentLayerPlan& p)const{
  if(!ready || canonical!=&c || parent!=&p)return false;
  uint32_t current[TT_Q38_LAYER_LINK_WORDS];if(!Describe(c,p,current))return false;
  for(unsigned i=0;i<TT_Q38_LAYER_LINK_WORDS;i++)if(current[i]!=descriptor[i])return false;
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  if(!ready || !canonical || !parent || !pages || count!=PioneerResidentBanks::Pages || !Matches(*canonical,*parent))return false;
  const auto* n=descriptor+64;
  unsigned first=n[10]*unsigned(canonical->Capacity/4096)+(n[11]-canonical->Base)/4096;
  if(first>=count || count-first<5)return false;
  for(unsigned i=0;i<5;i++)if(pages[first+i]!=1)return false;
  return true; // only added next norm; caller separately owes ALL parent pages
 }
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
};
#endif
