/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CONV_PLAN_H
#define PIONEER_Q38_CONV_PLAN_H
#include "PioneerQ38CanonicalModelPlan.h"
#include "PioneerQ38ConvShape.h"
// Canonical DATA recipe,NOT an owner or a current-state/producer grant.
// Default ProjectionMutableBanks DOES NOT reserve ConvKind. Its separate
// offline DECODER_RUNTIME role now reserves the actual canonical history;
// zero-init still does NOT grant a current-state/producer capability. The
// native consumer must retain it and consume the private QPS1 proof itself.
class PioneerQ38ConvPlan {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;uint32_t descriptor[64]={};bool attempted=false,ready=false;
 static bool Describe(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned first,unsigned position,
  unsigned bank,unsigned worker,const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase,uint32_t* d){
  if(!canonical.Ready() || layer>=64 || layer%4==3 || first>=10240 || (first&15) || position>=8192
   || bank>=7 || worker>=120 || !id.Valid() || !source || !source[0] || !phase || phase>=0x7fffffff)return false;
  const PioneerQ38CanonicalPart* learned=nullptr;const PioneerQ38CanonicalPart* previous=nullptr;
  for(unsigned i=0;i<canonical.Count();i++){
   const auto* p=canonical.Part(i);if(p->layer!=layer || first<p->firstRow || first-p->firstRow>p->rows || 16>p->rows-(first-p->firstRow))continue;
   if(p->kind==canonical.WeightKind && p->role==12){
    if(learned || p->type || p->rowBytes!=16)return false;
    learned=p;
   }
   if(p->kind==canonical.ConvKind){if(previous || p->role || p->type || p->rowBytes!=12)return false;previous=p;}
  }
  const auto* workspace=canonical.Workspace(bank);if(!learned || !previous || !workspace || workspace->bytes!=67108864)return false;
  for(unsigned i=0;i<64;i++)d[i]=0;
  d[0]=TT_Q38_CONV_MARKER;d[1]=1;d[2]=67108864;d[3]=bank;d[4]=canonical.Base+workspace->offset;
  d[5]=layer;d[6]=phase;d[7]=first;d[8]=PioneerQ38ProjectionRoute(bank);d[9]=PioneerQ38ConvPeer(worker);d[10]=worker;
  d[11]=learned->bank;d[12]=canonical.Base+learned->offset+(first-learned->firstRow)*16;d[13]=PioneerQ38ProjectionRoute(d[11]);
  d[14]=previous->bank;d[15]=canonical.Base+previous->offset+(first-previous->firstRow)*12;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];
  const uint64_t identity[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
  for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(identity[i]);d[33+i*2]=uint32_t(identity[i]>>32);}
  d[40]=position;d[41]=PioneerQ38ProjectionRoute(d[14]);d[42]=TT_Q38_CONV_NEXT+first*12;
  d[43]=TT_Q38_CONV_OUTPUT+first*4;d[44]=16;d[45]=PioneerQ38ProjectionResultOffset(6)+first*4;
  d[46]=192;d[47]=256;d[48]=3;d[49]=2;d[50]=8;d[51]=4;
  return PioneerQ38ConvShape(d);
 }
public:
 PioneerQ38ConvPlan()=default;
 PioneerQ38ConvPlan(const PioneerQ38ConvPlan&)=delete;
 PioneerQ38ConvPlan& operator=(const PioneerQ38ConvPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned first,unsigned position,unsigned bank,unsigned worker,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
  if(attempted)return false;
  attempted=true;plan=&canonical;
  ready=Describe(canonical,layer,first,position,bank,worker,id,source,phase,descriptor);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned first,unsigned position,unsigned bank,unsigned worker,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase)const{
  if(!ready || plan!=&canonical)return false;
  uint32_t current[64];
  if(!Describe(canonical,layer,first,position,bank,worker,id,source,phase,current))return false;
  for(unsigned i=0;i<64;i++)if(current[i]!=descriptor[i])return false;
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  if(!ready || !pages || count!=PioneerResidentBanks::Pages || !PioneerQ38ConvShape(descriptor))return false;
  unsigned at=descriptor[11]*unsigned(plan->Capacity/4096)+(descriptor[12]-plan->Base)/4096;
  return pages[at]==1; // 256B original group cannot cross a page
 }
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
};
#endif
