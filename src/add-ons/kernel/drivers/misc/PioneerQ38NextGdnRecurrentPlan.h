/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_GDN_RECURRENT_PLAN_H
#define PIONEER_Q38_NEXT_GDN_RECURRENT_PLAN_H
#if !defined(PIONEER_Q38_NEXT_GDN_NATIVE) || PIONEER_Q38_NEXT_GDN_NATIVE != 1
#error Exact separate native61 next-layer GDN required
#endif
#include "PioneerQ38GdnRecurrentPlan.h"
// Layer1/position0/current0 -> privateNEXT1 DATA only. Never retag the
// original layer0 plan or its native first-layer claim/publication.
class PioneerQ38NextGdnRecurrentPlan {
 PioneerQ38GdnInputPlan input;
 const PioneerQ38GdnBanks* state=nullptr;uint32_t descriptor[64]={};bool attempted=false,ready=false;
 static bool Describe(const PioneerQ38GdnBanks& state,const PioneerQ38GdnIdentity& id,unsigned half,unsigned phase,
  const uint32_t* in,uint32_t* d){
  if(half>=96 || !in || in[5]!=1 || in[40] || !state.InitializationPublished(id))return false;
  PioneerQ38GdnBankExtent current,next;
  if(!state.InitializeHalf(id,1,0,half,current) || !state.InitializeHalf(id,1,1,half,next)
   || current.bytes!=32768 || current.pages!=8 || next.bytes!=32768 || next.pages!=8)return false;
  for(unsigned i=0;i<64;i++)d[i]=0;
  d[0]=TT_Q38_RGDN_MARKER;d[1]=1;d[2]=128;d[3]=64;
  d[4]=current.bank;d[5]=current.address;d[6]=next.bank;d[7]=next.address;
  d[8]=phase;d[9]=half;d[10]=1;d[11]=0;d[12]=current.route;d[13]=next.route;d[14]=32768;d[15]=in[9];
  for(unsigned i=16;i<40;i++)d[i]=in[i];
  return PioneerQ38GdnRecurrentShape(in,d);
 }
public:
 PioneerQ38NextGdnRecurrentPlan()=default;
 PioneerQ38NextGdnRecurrentPlan(const PioneerQ38NextGdnRecurrentPlan&)=delete;
 PioneerQ38NextGdnRecurrentPlan& operator=(const PioneerQ38NextGdnRecurrentPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38GdnBanks& owner,unsigned half,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
  if(attempted)return false;
  attempted=true;state=&owner;
  ready=input.Build(canonical,1,half,0,bank,id,source,phase) && Describe(owner,id,half,phase,input.Descriptor(),descriptor);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38GdnBanks& owner,unsigned half,unsigned bank,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase)const{
  if(!ready || state!=&owner || !input.Matches(canonical,1,half,0,bank,id,source,phase))return false;
  uint32_t expected[64];if(!Describe(owner,id,half,phase,input.Descriptor(),expected))return false;
  for(unsigned i=0;i<64;i++)if(expected[i]!=descriptor[i])return false;
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{return ready && input.LearnedPagesPresent(pages,count);}
 const uint32_t* Descriptor()const{return ready?input.Descriptor():nullptr;}
 const uint32_t* StateDescriptor()const{return ready?descriptor:nullptr;}
};
#endif
