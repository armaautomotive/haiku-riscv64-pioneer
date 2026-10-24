/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_GATED_PLAN_H
#define PIONEER_Q38_GDN_GATED_PLAN_H
#include "PioneerQ38GdnInputPlan.h"
#include "PioneerQ38GdnGatedShape.h"
// DATA factory only;future SAME native owner must independently prove the
// retained role5 Z producer,role9 uploaded original page,ALL peers' fresh
// zero readiness cells BEFORE release,and no epoch reuse until ALL holds.
class PioneerQ38GdnGatedPlan {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;uint32_t descriptor[64]={};bool attempted=false,ready=false;
 static const PioneerQ38CanonicalPart* Norm(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer){
  if(!canonical.Ready() || layer>=64 || layer%4==3)return nullptr;
  const PioneerQ38CanonicalPart* norm=nullptr;
  for(unsigned i=0;i<canonical.Count();i++){
   const auto* p=canonical.Part(i);
   if(p->kind!=canonical.WeightKind || p->layer!=layer || p->role!=9)continue;
   if(norm || p->type || p->firstRow || p->rows!=1 || p->rowBytes!=512 || p->bytes!=512 || p->allocated!=4096)return nullptr;
   norm=p;
  }
  return norm;
 }
 static bool Describe(const PioneerQ38CanonicalModelPlan& canonical,const uint32_t* input,const uint32_t* state,uint32_t* d){
  if(!canonical.Ready() || !PioneerQ38GdnRecurrentShape(input,state))return false;
  const auto* workspace=canonical.Workspace(input[3]);
  if(!workspace || canonical.Base+workspace->offset!=input[4])return false;
  const auto* norm=Norm(canonical,input[5]);
  if(!norm)return false;
  for(unsigned i=0;i<64;i++)d[i]=input[i];
  d[0]=TT_Q38_GDN_GATED_MARKER;d[11]=norm->bank;d[12]=canonical.Base+norm->offset;d[13]=PioneerQ38ProjectionRoute(norm->bank);
  d[14]=(input[7]^1)+1;d[15]=PioneerQ38ConvPeer(d[14]);d[41]=state[11];
  d[42]=PioneerQ38ProjectionResultOffset(5)+(input[7]>>1)*512;d[43]=TT_Q38_GDN_GATED_OUTPUT+input[7]*256;
  d[44]=TT_Q38_GDN_GATED_READY;d[45]=0x8e800;d[46]=128;d[47]=64;d[48]=512;d[49]=512;d[50]=256;d[51]=TT_Q38_GDN_GATED_POLLS;
  for(unsigned i=52;i<64;i++)d[i]=0;
  return PioneerQ38GdnGatedShape(d,input,state);
 }
public:
 static bool LearnedPagePresentBeforeState(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,const uint8_t* pages,unsigned count){
  if(!pages || count!=PioneerResidentBanks::Pages)return false;
  const auto* norm=Norm(canonical,layer);if(!norm)return false;
  unsigned page=norm->bank*unsigned(canonical.Capacity/4096)+norm->offset/4096;
  return pages[page]==1; // CPU precondition only,never an authenticated source/native capability
 }
 PioneerQ38GdnGatedPlan()=default;PioneerQ38GdnGatedPlan(const PioneerQ38GdnGatedPlan&)=delete;
 PioneerQ38GdnGatedPlan& operator=(const PioneerQ38GdnGatedPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,const uint32_t* input,const uint32_t* state){
  if(attempted)return false;
  attempted=true;plan=&canonical;ready=Describe(canonical,input,state,descriptor);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,const uint32_t* input,const uint32_t* state)const{
  if(!ready || plan!=&canonical)return false;
  uint32_t current[64];if(!Describe(canonical,input,state,current))return false;
  for(unsigned i=0;i<64;i++)if(current[i]!=descriptor[i])return false;
  return true;
 }
 bool LearnedPagePresent(const uint8_t* pages,unsigned count)const{
  if(!ready || !pages || count!=PioneerResidentBanks::Pages)return false;
  unsigned page=descriptor[11]*unsigned(plan->Capacity/4096)+(descriptor[12]-plan->Base)/4096;
  return pages[page]==1; // DATA only,not an actual uploaded native/source grant
 }
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
};
#endif
