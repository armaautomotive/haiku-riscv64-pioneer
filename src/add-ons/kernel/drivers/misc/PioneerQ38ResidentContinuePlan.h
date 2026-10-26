/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_CONTINUE_PLAN_H
#define PIONEER_Q38_RESIDENT_CONTINUE_PLAN_H
#include "PioneerQ38ResidentAdvancePlan.h"
#include "PioneerQ38ResidentContinueShape.h"
// HEAP immutable canonical DATA. The native source/lifetime/journal and
// immutable device recipe upload are OWED, not supplied by these pointers.
class PioneerQ38ResidentContinuePlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38ResidentLayerPlan* parent=nullptr;
 const PioneerQ38ResidentLayerLinkPlan* link=nullptr;
 const PioneerQ38ResidentAdvancePlan* advance=nullptr;
 PioneerQ38ResidentLayerPlan next;
 uint32_t descriptor[TT_Q38_CONTINUE_WORDS]={};bool attempted=false,ready=false;
public:
 PioneerQ38ResidentContinuePlan()=default;
 PioneerQ38ResidentContinuePlan(const PioneerQ38ResidentContinuePlan&)=delete;
 PioneerQ38ResidentContinuePlan& operator=(const PioneerQ38ResidentContinuePlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38ResidentLayerPlan& p,
  const PioneerQ38ResidentLayerLinkPlan& n,const PioneerQ38ResidentAdvancePlan& a){
  if(attempted)return false;
  attempted=true;
  if(!a.Matches(c,p,n))return false;
  const auto* h=a.Descriptor();const auto* m=p.Manifest();if(h[3]>=63 || h[11]>=0x7fffffdf)return false;
  const PioneerQ38GdnIdentity id={uint64_t(h[32])|(uint64_t(h[33])<<32),uint64_t(h[34])|(uint64_t(h[35])<<32),
   uint64_t(h[36])|(uint64_t(h[37])<<32),uint64_t(h[38])|(uint64_t(h[39])<<32)};
  uint32_t zero[119]={};
  if(!next.Build(c,h[3],h[4],h[6],m[14],h[5],h[11]+1,id,m+TT_Q38_COHORT_SOURCE_WORD,zero))return false;
  canonical=&c;parent=&p;link=&n;advance=&a;auto* d=descriptor;
  d[0]=TT_Q38_CONTINUE_MARKER;d[1]=1;d[2]=TT_Q38_CONTINUE_WORDS;d[3]=h[3];d[4]=h[4];d[5]=h[5];d[6]=h[6];d[7]=h[7];d[8]=h[15];
  d[9]=h[11]+1;d[10]=m[14];d[11]=TT_Q38_CONTINUE_RECIPE_WORDS*4;d[12]=TT_Q38_CONTINUE_RECIPE_OFFSET;d[13]=19;d[14]=119;d[15]=TT_Q38_ADVANCE_RECEIPT_MARKER;
  for(unsigned i=16;i<40;i++)d[i]=h[i];
  const unsigned fields[]={TT_Q38_COHORT_WORDS,TT_Q38_OUTPUT_COHORT_WORDS,TT_Q38_RESIDENT_LAYER_WORDS,5120,6144,5120};
  for(unsigned i=0;i<6;i++)d[40+i]=fields[i];
  ready=PioneerQ38ResidentContinueShape(d,h,m) && PioneerQ38ResidentContinueRecipe(d,next.Manifest(),next.PostManifest(),next.LayerManifest(),m,nullptr);
  return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38ResidentLayerPlan& p,
  const PioneerQ38ResidentLayerLinkPlan& n,const PioneerQ38ResidentAdvancePlan& a)const{
  return ready && canonical==&c && parent==&p && link==&n && advance==&a && a.Matches(c,p,n) && next.CanonicalRecipe()
   && PioneerQ38ResidentContinueShape(descriptor,a.Descriptor(),p.Manifest())
   && PioneerQ38ResidentContinueRecipe(descriptor,next.Manifest(),next.PostManifest(),next.LayerManifest(),p.Manifest(),nullptr);
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  return ready && canonical && parent && link && advance && Matches(*canonical,*parent,*link,*advance)
   && parent->LearnedPagesPresent(pages,count) && link->LearnedPagesPresent(pages,count)
   && advance->LearnedPagesPresent(pages,count) && next.LearnedPagesPresent(pages,count);
 } // complete two-layer learned-page DATA only,not upload/auth/init/lifetime
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
 const PioneerQ38ResidentLayerPlan* Next()const{return ready?&next:nullptr;}
};
static_assert(sizeof(PioneerQ38ResidentContinuePlan)>524288 && sizeof(PioneerQ38ResidentContinuePlan)<1048576,"HEAP ONLY");
#endif
