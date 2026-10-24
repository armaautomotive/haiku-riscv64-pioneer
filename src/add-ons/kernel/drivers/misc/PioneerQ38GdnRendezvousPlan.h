/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_RENDEZVOUS_PLAN_H
#define PIONEER_Q38_GDN_RENDEZVOUS_PLAN_H
#include "PioneerQ38GdnGatedPlan.h"
#include "PioneerQ38GdnRendezvousShape.h"
// Canonical timed DATA only;SAME native source/clock/initialized owners required.
class PioneerQ38GdnRendezvousPlan {
 PioneerQ38GdnGatedPlan base;uint32_t descriptor[64]={};bool attempted=false,ready=false;
 static void Timed(const uint32_t* old,uint32_t* d){
  for(unsigned i=0;i<64;i++)d[i]=old[i];
  d[0]=TT_Q38_GDN_RENDEZVOUS_MARKER;d[51]=TT_Q38_GDN_RENDEZVOUS_POLLS;
  d[52]=uint32_t(PioneerQ38GdnRendezvousBudget());d[53]=uint32_t(PioneerQ38GdnRendezvousBudget()>>32);
  d[54]=4;d[55]=TT_Q38_GDN_RENDEZVOUS_CLOCK;
 }
public:
 PioneerQ38GdnRendezvousPlan()=default;PioneerQ38GdnRendezvousPlan(const PioneerQ38GdnRendezvousPlan&)=delete;
 PioneerQ38GdnRendezvousPlan& operator=(const PioneerQ38GdnRendezvousPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,const uint32_t* input,const uint32_t* state){
  if(attempted)return false;
  attempted=true;
  if(!base.Build(canonical,input,state))return false;
  Timed(base.Descriptor(),descriptor);ready=PioneerQ38GdnRendezvousShape(descriptor,input,state);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,const uint32_t* input,const uint32_t* state)const{
  if(!ready || !base.Matches(canonical,input,state))return false;
  uint32_t expected[64];Timed(base.Descriptor(),expected);
  for(unsigned i=0;i<64;i++)if(expected[i]!=descriptor[i])return false;
  return PioneerQ38GdnRendezvousShape(descriptor,input,state);
 }
 bool LearnedPagePresent(const uint8_t* pages,unsigned count)const{return ready && base.LearnedPagePresent(pages,count);}
 const uint32_t* Descriptor()const{return ready?descriptor:nullptr;}
};
#endif
