/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_ADVANCE_PLAN_H
#define PIONEER_Q38_RESIDENT_ADVANCE_PLAN_H
#include "PioneerQ38ResidentLayerLinkPlan.h"
#include "PioneerQ38ProjectionJobs.h"
#include "PioneerQ38ResidentAdvanceShape.h"
// HEAP geometry only. Not an old NextProjection owner/producer alias.
class PioneerQ38ResidentAdvancePlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;const PioneerQ38ResidentLayerPlan* parent=nullptr;
 const PioneerQ38ResidentLayerLinkPlan* link=nullptr;
 PioneerQ38ProjectionJobs jobs[4];uint32_t descriptor[TT_Q38_ADVANCE_WORDS]={};bool attempted=false,ready=false;
public:
 PioneerQ38ResidentAdvancePlan()=default;
 PioneerQ38ResidentAdvancePlan(const PioneerQ38ResidentAdvancePlan&)=delete;
 PioneerQ38ResidentAdvancePlan& operator=(const PioneerQ38ResidentAdvancePlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38ResidentLayerPlan& p,const PioneerQ38ResidentLayerLinkPlan& next){
  if(attempted)return false;attempted=true;
  if(!next.Matches(c,p))return false;
  const auto* n=next.Descriptor();if(!n || n[4]%4==3 || n[10]>=0x7ffffff8)return false;
  canonical=&c;parent=&p;link=&next;auto* d=descriptor;
  d[0]=TT_Q38_ADVANCE_MARKER;d[1]=1;d[2]=TT_Q38_ADVANCE_WORDS;d[3]=n[4];d[4]=n[5];d[5]=n[6];d[6]=n[7];d[7]=n[8];
  d[8]=n[10];d[9]=n[10]+1;d[10]=n[10]+2;d[11]=n[10]+5;d[12]=4;d[13]=20;d[14]=244;d[15]=PioneerQ38ProjectionRoute(d[6]);
  for(unsigned i=16;i<40;i++)d[i]=n[i];
  d[40]=TT_Q38_LAYER_LINK_RECEIPT_MARKER;d[41]=8;d[42]=5;d[43]=TT_Q38_RESIDENT_COHORT_MARKER;
  for(unsigned role=0;role<4;role++){
   unsigned semantic=PioneerQ38ResidentAdvanceRole(role);auto& j=jobs[role];if(!j.Build(c,d[3],semantic,d[4]) || j.Type()!=8 || j.Inputs()!=5120 || j.Waves()!=1)return false;
   auto* r=d+64+role*TT_Q38_ADVANCE_ROLE_WORDS;r[0]=semantic;r[1]=8;r[2]=j.Count();r[3]=5120;r[4]=j.Outputs();r[5]=d[10]+role;
   r[6]=PioneerQ38ProjectionResultOffset(semantic);r[7]=j.Outputs()*4;
   for(unsigned w=0;w<j.Count();w++){
    const auto* part=j.Part(w);auto* out=r+64+w*8;if(!part || part->offset>UINT32_MAX-c.Base)return false;
    out[0]=c.Base+uint32_t(part->offset);out[1]=part->bank;out[2]=part->groups;out[4]=PioneerQ38ProjectionRoute(part->bank);
    out[5]=PioneerQ38CohortSource(p.Manifest(),w+1)[0];r[1016+w]=part->firstRow;
   }
  }
  ready=PioneerQ38ResidentAdvanceShape(d,n,p.Manifest());return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38ResidentLayerPlan& p,const PioneerQ38ResidentLayerLinkPlan& next)const{
  if(!ready || canonical!=&c || parent!=&p || link!=&next
   || !PioneerQ38ResidentAdvanceShape(descriptor,next.Descriptor(),p.Manifest()) || !next.Matches(c,p))return false;
  for(unsigned role=0;role<4;role++){
   const auto* r=PioneerQ38ResidentAdvanceRecord(descriptor,role);const auto& j=jobs[role];
   if(!j.Matches(c,descriptor[3],r[0],descriptor[4]) || j.Count()!=r[2] || j.Outputs()!=r[4])return false;
   for(unsigned w=0;w<j.Count();w++){
    const auto* q=j.Part(w);const auto* a=r+64+w*8;
    if(a[0]!=c.Base+q->offset || a[1]!=q->bank || a[2]!=q->groups || r[1016+w]!=q->firstRow)return false;
   }
  }
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  if(!ready || !canonical || !parent || !link || !pages || !Matches(*canonical,*parent,*link))return false;
  for(unsigned role=0;role<4;role++)if(!jobs[role].LearnedPagesPresent(pages,count))return false;
  return true; // added projection weights only; not parent/norm/auth/initialized ownership
 }
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
};
static_assert(sizeof(PioneerQ38ResidentAdvancePlan)>114688 && sizeof(PioneerQ38ResidentAdvancePlan)<131072,"HEAP ONLY");
#endif
