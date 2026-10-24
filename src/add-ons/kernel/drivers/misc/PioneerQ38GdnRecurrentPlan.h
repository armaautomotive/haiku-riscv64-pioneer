/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_RECURRENT_PLAN_H
#define PIONEER_Q38_GDN_RECURRENT_PLAN_H
#include "PioneerQ38GdnInputPlan.h"
#include "PioneerQ38GdnBanks.h"
#include "PioneerQ38GdnRecurrentShape.h"
// Immutable paired DATA only. Actual SAME native initialized-state/retained
// CBO producer/cookie/source/clock/ALL hosts scope still authorizes execution.
// First layer0/token-position0/current-slot0 -> private-next-slot1 ONLY.
class PioneerQ38GdnRecurrentPlan {
 PioneerQ38GdnInputPlan input;
 const PioneerQ38GdnBanks* state=nullptr;uint32_t descriptor[64]={};bool attempted=false,ready=false;
 static bool Describe(const PioneerQ38GdnBanks& state,const PioneerQ38GdnIdentity& id,unsigned half,unsigned phase,
  const uint32_t* input,uint32_t* output){
  if(half>=96 || !input || !state.InitializationPublished(id))return false;
  PioneerQ38GdnBankExtent current,next;
  if(!state.InitializeHalf(id,0,0,half,current) || !state.InitializeHalf(id,0,1,half,next)
   || current.bytes!=32768 || current.pages!=8 || next.bytes!=32768 || next.pages!=8)return false;
  for(unsigned i=0;i<64;i++)output[i]=0;
  output[0]=TT_Q38_RGDN_MARKER;output[1]=1;output[2]=128;output[3]=64;
  output[4]=current.bank;output[5]=current.address;output[6]=next.bank;output[7]=next.address;
  output[8]=phase;output[9]=half;output[10]=0;output[11]=0;output[12]=current.route;output[13]=next.route;
  output[14]=32768;output[15]=input[9];
  for(unsigned i=16;i<40;i++)output[i]=input[i];
  return PioneerQ38GdnRecurrentShape(input,output);
 }
public:
 PioneerQ38GdnRecurrentPlan()=default;
 PioneerQ38GdnRecurrentPlan(const PioneerQ38GdnRecurrentPlan&)=delete;
 PioneerQ38GdnRecurrentPlan& operator=(const PioneerQ38GdnRecurrentPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38GdnBanks& owner,unsigned half,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
  if(attempted)return false;
  attempted=true;state=&owner;
  ready=input.Build(canonical,0,half,0,bank,id,source,phase) && Describe(owner,id,half,phase,input.Descriptor(),descriptor);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38GdnBanks& owner,unsigned half,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase)const{
  if(!ready || state!=&owner || !input.Matches(canonical,0,half,0,bank,id,source,phase))return false;
  uint32_t current[64];if(!Describe(owner,id,half,phase,input.Descriptor(),current))return false;
  for(unsigned i=0;i<64;i++)if(current[i]!=descriptor[i])return false;
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{return ready && input.LearnedPagesPresent(pages,count);}
 const uint32_t* Descriptor()const{return ready?input.Descriptor():nullptr;}
 const uint32_t* StateDescriptor()const{return ready?descriptor:nullptr;}
};
#endif
