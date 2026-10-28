/* SPDX-License-Identifier: MIT */
#include "PioneerQ38RecurrentPrefixFrozenPlanRole.h"
#ifndef PIONEER_Q38_RECURRENT_PREFIX_FROZEN_PLAN_H
#define PIONEER_Q38_RECURRENT_PREFIX_FROZEN_PLAN_H
#include "../../../../../tools/pioneer/PioneerQ38RecurrentPrefixPlan.h"
#include <string.h>
#include <type_traits>
class PioneerQ38RecurrentPrefixOwner;
// PRIVATE geometry equality only. Inline in the retained execution owner,
// never a caller-created ready bit,cache pointer or native lifetime grant.
// All mutable source/FD/team/model/clock/hosts/cohort/MMIO guards stay live.
class PioneerQ38RecurrentPrefixFrozenPlan {
 friend class PioneerQ38RecurrentPrefixOwner;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38RecurrentPrefixPlan* plan=nullptr;
 unsigned char canonicalBytes[sizeof(PioneerQ38CanonicalModelPlan)];
 unsigned char planBytes[sizeof(PioneerQ38RecurrentPrefixPlan)];
 bool attempted=false,ready=false;
 PioneerQ38RecurrentPrefixFrozenPlan()=default;
 ~PioneerQ38RecurrentPrefixFrozenPlan()=default;
 PioneerQ38RecurrentPrefixFrozenPlan(const PioneerQ38RecurrentPrefixFrozenPlan&)=delete;
 PioneerQ38RecurrentPrefixFrozenPlan& operator=(const PioneerQ38RecurrentPrefixFrozenPlan&)=delete;
 bool SameBytes(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& p)const{
  return canonical==&c && plan==&p && !memcmp(canonicalBytes,&c,sizeof(c)) && !memcmp(planBytes,&p,sizeof(p));
 }
 __attribute__((noinline)) bool Capture(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& p){
  if(attempted)return false;
  attempted=true;if(!c.Ready() || !p.Matches(c))return false;
  memcpy(canonicalBytes,&c,sizeof(c));memcpy(planBytes,&p,sizeof(p));canonical=&c;plan=&p;
  if(!p.Matches(c) || !SameBytes(c,p)){canonical=nullptr;plan=nullptr;return false;}
  ready=true;return true;
 }
 __attribute__((noinline)) bool Matches(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& p)const{return ready && SameBytes(c,p);}
}; // friend owner alone can capture/query;no public constructor or ready accessor
static_assert(std::is_trivially_copyable<PioneerQ38CanonicalModelPlan>::value && std::is_trivially_copyable<PioneerQ38RecurrentPrefixPlan>::value,
 "exact frozen geometry requires fully inline,trivially-copyable data and separately audited pointee closure");
static_assert(sizeof(PioneerQ38RecurrentPrefixFrozenPlan)>3*1048576 && sizeof(PioneerQ38RecurrentPrefixFrozenPlan)<7*1048576,"HEAP OWNED,never a kernel stack object");
#endif
