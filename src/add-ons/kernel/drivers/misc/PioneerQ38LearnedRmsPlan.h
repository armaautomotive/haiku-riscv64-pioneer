/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LEARNED_RMS_PLAN_H
#define PIONEER_Q38_LEARNED_RMS_PLAN_H
#include "PioneerQ38CanonicalModelPlan.h"
#include "PioneerQ38LearnedRmsShape.h"
// Pointer-bound canonical geometry ONLY. Actual private retained producer,
// source/learned ledger/initialized memory/clock/execution lifetime OWED.
class PioneerQ38LearnedRmsPlan {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;uint32_t descriptor[64]={};bool attempted=false,ready=false;
 static bool Describe(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned role,unsigned position,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase,uint32_t* d){
  if(!canonical.Ready() || layer>64 || (layer<64?role>1:role!=2) || position>=8192 || bank>=7
   || !id.Valid() || !source || !source[0] || !phase || phase>=0x7fffffff)return false;
  const PioneerQ38CanonicalPart* norm=nullptr;
  for(unsigned i=0;i<canonical.Count();i++){
   const auto* p=canonical.Part(i);if(p->kind!=canonical.WeightKind || p->layer!=layer || p->role!=role)continue;
   if(norm || p->type || p->firstRow || p->rows!=1 || p->rowBytes!=20480 || p->bytes!=20480 || p->allocated!=20480)return false;
   norm=p;
  }
  const auto* workspace=canonical.Workspace(bank);if(!norm || !workspace || workspace->bytes!=67108864)return false;
  for(unsigned i=0;i<64;i++)d[i]=0;
  d[0]=TT_Q38_RMS_MARKER;d[1]=1;d[2]=67108864;d[3]=bank;d[4]=canonical.Base+workspace->offset;
  d[5]=layer;d[6]=phase;d[7]=role;d[8]=PioneerQ38ProjectionRoute(bank);d[9]=129;
  d[10]=norm->bank;d[11]=canonical.Base+norm->offset;d[12]=PioneerQ38ProjectionRoute(norm->bank);d[13]=20480;d[14]=5120;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];
  const uint64_t identity[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
  for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(identity[i]);d[33+i*2]=uint32_t(identity[i]>>32);}
  d[40]=position;d[42]=TT_Q38_RMS_BACKUP;d[43]=20480;d[44]=0x358637bd;d[45]=5120;d[46]=4;d[47]=4;d[48]=640;d[49]=640;
  return PioneerQ38LearnedRmsShape(d);
 }
public:
 PioneerQ38LearnedRmsPlan()=default;
 PioneerQ38LearnedRmsPlan(const PioneerQ38LearnedRmsPlan&)=delete;
 PioneerQ38LearnedRmsPlan& operator=(const PioneerQ38LearnedRmsPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned role,unsigned position,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
  if(attempted)return false;
  attempted=true;plan=&canonical;ready=Describe(canonical,layer,role,position,bank,id,source,phase,descriptor);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned role,unsigned position,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase)const{
  if(!ready || plan!=&canonical)return false;
  uint32_t current[64];
  if(!Describe(canonical,layer,role,position,bank,id,source,phase,current))return false;
  for(unsigned i=0;i<64;i++)if(current[i]!=descriptor[i])return false;
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* initialized,unsigned pages)const{
  if(!ready || !initialized || pages!=PioneerResidentBanks::Pages || !PioneerQ38LearnedRmsShape(descriptor))return false;
  unsigned first=descriptor[10]*unsigned(plan->Capacity/4096)+(descriptor[11]-plan->Base)/4096;
  for(unsigned i=0;i<5;i++)if(initialized[first+i]!=1)return false;
  return true;
 }
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
};
#endif
