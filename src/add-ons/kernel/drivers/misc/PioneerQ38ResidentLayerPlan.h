/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_PLAN_H
#define PIONEER_Q38_RESIDENT_LAYER_PLAN_H
#include "PioneerQ38ResidentOutputPlan.h"
#include "PioneerQ38ResidentLayerShape.h"
#include "PioneerQ38FfnCanonicalJobs.h"
// HEAP ONLY;canonical immutable metadata,not source/initialized/producer or
// native lifetime authority. New owner must retain all resources through
// residual+RMS+FFN+residual,not borrow an old CLOSED receipt as permission.
class PioneerQ38ResidentLayerPlan {
 PioneerQ38ResidentOutputPlan output;
 PioneerQ38FfnCanonicalJobs ffn;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38CanonicalPart* norm=nullptr;
 uint32_t layer[TT_Q38_RESIDENT_LAYER_WORDS]={};
 bool attempted=false,ready=false;
public:
 PioneerQ38ResidentLayerPlan()=default;
 PioneerQ38ResidentLayerPlan(const PioneerQ38ResidentLayerPlan&)=delete;
 PioneerQ38ResidentLayerPlan& operator=(const PioneerQ38ResidentLayerPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& plan,unsigned selected,unsigned position,unsigned bank,unsigned slot,
  unsigned token,unsigned phase,const PioneerQ38GdnIdentity& id,const uint32_t* sources,const uint32_t* sequences){
  if(attempted)return false;
  attempted=true;
  if(phase>=0x7ffffff0 || !output.Build(plan,selected,position,bank,slot,token,phase,id,sources,sequences)
   || !ffn.Build(plan,selected,position))return false;
  canonical=&plan;
  for(unsigned i=0;i<plan.Count();i++){
   const auto* p=plan.Part(i);
   if(p->kind!=plan.WeightKind || p->layer!=selected || p->role!=1)continue;
   if(norm || p->type || p->rows!=1 || p->firstRow || p->rowBytes!=20480 || p->bytes!=20480 || p->allocated!=20480)return false;
   norm=p;
  }
  if(!norm)return false;
  const auto* m=output.Manifest();const auto* q=output.PostManifest();auto* l=layer;
  l[0]=TT_Q38_RESIDENT_LAYER_MARKER;l[1]=1;l[2]=TT_Q38_RESIDENT_LAYER_WORDS;l[3]=selected;l[4]=position;l[5]=phase;
  l[6]=q[7]+1;l[7]=q[7]+2;l[8]=bank;l[9]=m[8];l[10]=m[9];l[11]=token;
  l[12]=norm->bank;l[13]=plan.Base+norm->offset;l[14]=PioneerQ38ProjectionRoute(norm->bank);l[15]=20480;
  for(unsigned i=16;i<40;i++)l[i]=m[i];
  l[40]=TT_Q38_PROJECTION_OUTPUT_OFFSET;l[41]=98304;l[42]=5120;l[43]=TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER;
  l[44]=q[7];l[45]=0x358637bd;l[46]=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER;l[47]=TT_Q38_RESIDENT_LAYER_NORM_MARKER;
  auto* f=l+64;f[0]=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER;f[1]=3;f[2]=67108864;f[3]=bank;f[4]=m[8];f[5]=1;f[6]=l[7];f[7]=3;
  f[8]=m[9];f[9]=129;f[10]=TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+256;f[11]=4352;f[12]=TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+768;
  f[13]=f[6]+5;f[14]=1;for(unsigned i=16;i<40;i++)f[i]=m[i];f[40]=position;f[41]=token;f[42]=selected;
  for(unsigned role=0;role<3;role++){
   const auto* job=ffn.Job(role);if(!job || job->peers!=119 || job->type!=12)return false;
   auto* r=f+PioneerQ38FfnResidualGraphRoleOffset(role);r[0]=role;r[1]=12;r[2]=119;r[3]=role==2?17408:5120;r[4]=role==2?5120:17408;
   for(unsigned w=0;w<119;w++){
    const auto& p=job->part[w];if(p.offset>UINT32_MAX-plan.Base)return false;
    auto* d=r+16+w*8;d[0]=plan.Base+uint32_t(p.offset);d[1]=p.bank;d[2]=p.groups;d[4]=PioneerQ38ProjectionRoute(p.bank);
    d[5]=sources[(w+1)*16];r[16+119*8+w]=job->first[w];
   }
  }
  ready=PioneerQ38ResidentLayerShape(l,m,q);
  return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& plan,unsigned selected,unsigned position,unsigned bank,unsigned slot,
  unsigned token,unsigned phase,const PioneerQ38GdnIdentity& id,const uint32_t* sources,const uint32_t* sequences)const{
  if(!ready || canonical!=&plan || !norm || !output.Matches(plan,selected,position,bank,slot,token,phase,id,sources,sequences)
   || !ffn.Matches(plan,selected,position) || !PioneerQ38ResidentLayerShape(layer,output.Manifest(),output.PostManifest())
   || layer[12]!=norm->bank || layer[13]!=plan.Base+norm->offset)return false;
  for(unsigned role=0;role<3;role++){
   const auto* j=ffn.Job(role);const auto* r=layer+64+PioneerQ38FfnResidualGraphRoleOffset(role);
   if(!j)return false;
   for(unsigned w=0;w<119;w++){
    const auto* p=r+16+w*8;
    if(p[0]!=plan.Base+j->part[w].offset || p[1]!=j->part[w].bank || p[2]!=j->part[w].groups || r[16+119*8+w]!=j->first[w])return false;
   }
  }
  return true;
 }
 const uint32_t* Manifest()const{return ready?output.Manifest():nullptr;}
 bool CanonicalRecipe()const{
  if(!ready || !canonical || !norm)return false;
  const auto* m=output.Manifest();
  const PioneerQ38GdnIdentity id={uint64_t(m[32])|(uint64_t(m[33])<<32),uint64_t(m[34])|(uint64_t(m[35])<<32),
   uint64_t(m[36])|(uint64_t(m[37])<<32),uint64_t(m[38])|(uint64_t(m[39])<<32)};
  return Matches(*canonical,m[3],m[4],m[7],m[14],layer[11],m[5],id,
   m+TT_Q38_COHORT_SOURCE_WORD,m+TT_Q38_COHORT_SEQUENCE_WORD);
 } // necessary fixed DATA recipe only;never native/source/initialized authority
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  if(!ready || !canonical || !norm || !pages || count!=PioneerResidentBanks::Pages
   || !output.LearnedPagesPresent(pages,count) || !ffn.LearnedPagesPresent(pages,count))return false;
  const auto* m=output.Manifest();
  const PioneerQ38GdnIdentity id={uint64_t(m[32])|(uint64_t(m[33])<<32),uint64_t(m[34])|(uint64_t(m[35])<<32),
   uint64_t(m[36])|(uint64_t(m[37])<<32),uint64_t(m[38])|(uint64_t(m[39])<<32)};
  if(!Matches(*canonical,m[3],m[4],m[7],m[14],layer[11],m[5],id,
    m+TT_Q38_COHORT_SOURCE_WORD,m+TT_Q38_COHORT_SEQUENCE_WORD))return false;
  unsigned first=norm->bank*unsigned(canonical->Capacity/4096)+norm->offset/4096,n=norm->allocated/4096;
  if(first>=count || n>count-first)return false;
  for(unsigned i=0;i<n;i++)if(pages[first+i]!=1)return false;
  return true; // DATA presence only,not original-source authentication/initialization
 }
 const uint32_t* PostManifest()const{return ready?output.PostManifest():nullptr;}
 const uint32_t* LayerManifest()const{return ready?layer:nullptr;}
};
static_assert(sizeof(PioneerQ38ResidentLayerPlan)>524288 && sizeof(PioneerQ38ResidentLayerPlan)<1048576,"complete layer recipes HEAP ONLY");
#endif
