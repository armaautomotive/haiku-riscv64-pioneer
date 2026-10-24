/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDUAL_RMS_PLAN_H
#define PIONEER_Q38_RESIDUAL_RMS_PLAN_H
#include "PioneerQ38LearnedRmsPlan.h"
#include "PioneerQ38ResidualRmsShape.h"
// Canonical DATA only. A future SAME native owner must separately claim
// genuine completed ssm_out AND preserved raw residual before execution.
class PioneerQ38ResidualRmsPlan {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;uint32_t descriptor[64]={};bool attempted=false,ready=false;
 static bool Describe(const PioneerQ38CanonicalModelPlan& canonical,unsigned token,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase,uint32_t* d){
  if(token>=248320 || phase<2 || phase>=0x7fffffff)return false;
  PioneerQ38LearnedRmsPlan norm;if(!norm.Build(canonical,0,1,0,bank,id,source,phase))return false;
  for(unsigned i=0;i<64;i++)d[i]=norm.Descriptor()[i];
  d[0]=TT_Q38_RESIDUAL_RMS_MARKER;d[15]=TT_Q38_GDN_OUTPUT_RESULT;d[41]=token;
  d[46]=6;d[48]=960;d[51]=TT_Q38_RMS_BACKUP;d[52]=TT_Q38_GDN_OUTPUT_GRAPH_MARKER;d[53]=phase-1;d[54]=TT_Q38_RMS_MARKER;
  return PioneerQ38ResidualRmsShape(d);
 }
public:
 PioneerQ38ResidualRmsPlan()=default;
 PioneerQ38ResidualRmsPlan(const PioneerQ38ResidualRmsPlan&)=delete;
 PioneerQ38ResidualRmsPlan& operator=(const PioneerQ38ResidualRmsPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,unsigned token,unsigned bank,const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
  if(attempted)return false;
  attempted=true;plan=&canonical;ready=Describe(canonical,token,bank,id,source,phase,descriptor);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,unsigned token,unsigned bank,const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase)const{
  if(!ready || plan!=&canonical)return false;
  uint32_t current[64];if(!Describe(canonical,token,bank,id,source,phase,current))return false;
  for(unsigned i=0;i<64;i++)if(current[i]!=descriptor[i])return false;
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* ledger,unsigned pages)const{
  if(!ready || !ledger || pages!=PioneerResidentBanks::Pages || !PioneerQ38ResidualRmsShape(descriptor))return false;
  unsigned first=descriptor[10]*unsigned(plan->Capacity/4096)+(descriptor[11]-plan->Base)/4096;
  for(unsigned i=0;i<5;i++)if(ledger[first+i]!=1)return false;
  return true;
 }
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
};
#endif
