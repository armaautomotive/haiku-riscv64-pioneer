/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_RESIDUAL_RMS_PLAN_H
#define PIONEER_Q38_NEXT_RESIDUAL_RMS_PLAN_H
#include "PioneerQ38LearnedRmsPlan.h"
#include "PioneerQ38NextResidualRmsShape.h"
// Canonical DATA only. A real native consumer must FIRST claim the SAME
// genuine QOP2 output/raw-residual producer,not accept this as readiness.
class PioneerQ38NextResidualRmsPlan {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;uint32_t descriptor[64]={};bool attempted=false,ready=false;
 static bool Describe(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned position,unsigned token,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase,uint32_t* d){
  if(!layer || layer>=64 || (layer&3)==3 || position>=8192 || token>=248320 || phase<2 || phase>=0x7fffffff)return false;
  PioneerQ38LearnedRmsPlan norm;if(!norm.Build(canonical,layer,1,position,bank,id,source,phase))return false;
  for(unsigned i=0;i<64;i++)d[i]=norm.Descriptor()[i];
  d[0]=TT_Q38_NEXT_RESIDUAL_RMS_MARKER;d[15]=TT_Q38_NEXT_GDN_OUTPUT_RESULT;d[41]=token;
  d[46]=6;d[48]=960;d[50]=13;d[51]=TT_Q38_RMS_BACKUP;d[52]=TT_Q38_NEXT_GDN_OUTPUT_GRAPH_MARKER;d[53]=phase-1;d[54]=TT_Q38_RMS_MARKER;
  return PioneerQ38NextResidualRmsShape(d);
 }
public:
 PioneerQ38NextResidualRmsPlan()=default;
 PioneerQ38NextResidualRmsPlan(const PioneerQ38NextResidualRmsPlan&)=delete;
 PioneerQ38NextResidualRmsPlan& operator=(const PioneerQ38NextResidualRmsPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned position,unsigned token,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
  if(attempted)return false;
  attempted=true;plan=&canonical;ready=Describe(canonical,layer,position,token,bank,id,source,phase,descriptor);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned position,unsigned token,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase)const{
  if(!ready || plan!=&canonical)return false;
  uint32_t current[64];if(!Describe(canonical,layer,position,token,bank,id,source,phase,current))return false;
  for(unsigned i=0;i<64;i++)if(current[i]!=descriptor[i])return false;
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* ledger,unsigned pages)const{
  if(!ready || !plan || !plan->Ready() || !ledger || pages!=PioneerResidentBanks::Pages || !PioneerQ38NextResidualRmsShape(descriptor))return false;
  const PioneerQ38CanonicalPart* norm=nullptr;
  for(unsigned i=0;i<plan->Count();i++){
   const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=descriptor[5] || p->role!=1)continue;
   if(norm || p->type || p->firstRow || p->rows!=1 || p->rowBytes!=20480 || p->bytes!=20480 || p->allocated!=20480)return false;
   norm=p;
  }
  const auto* workspace=plan->Workspace(descriptor[3]);
  if(!norm || norm->bank!=descriptor[10] || plan->Base+norm->offset!=descriptor[11] || !workspace
   || workspace->bytes!=67108864 || workspace->allocated!=67108864 || plan->Base+workspace->offset!=descriptor[4])return false;
  unsigned first=norm->bank*unsigned(plan->Capacity/4096)+norm->offset/4096;
  if(first>=pages || pages-first<5)return false;
  for(unsigned i=0;i<5;i++)if(ledger[first+i]!=1)return false;
  return true;
 }
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
};
#endif
