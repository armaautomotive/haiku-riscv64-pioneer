/* SPDX-License-Identifier: MIT */
// HOST-ONLY experiment,not a native scope/ready cache/producer or IO grant.
// Recheck on late inclusion;do not add this to a driver to bypass its guards.
#if !defined(PIONEER_Q38_RECURRENT_PREFIX_PLAN_MIRROR_PROTOTYPE) || PIONEER_Q38_RECURRENT_PREFIX_PLAN_MIRROR_PROTOTYPE != 1
#error Exact standalone prefix plan mirror experiment required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RECURRENT_PREFIX_EMIT_CONTEXT_BODY) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CONTROL_COMPILE_ONLY) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST) || defined(PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE) || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Mirror prototype cannot enter native,kernel,client,oracle,image or Work roles,including defined0
#endif
#ifndef PIONEER_Q38_RECURRENT_PREFIX_PLAN_MIRROR_H
#define PIONEER_Q38_RECURRENT_PREFIX_PLAN_MIRROR_H
#include "PioneerQ38RecurrentPrefixPlan.h"
#include <string.h>
#include <type_traits>
// All geometry lives inline in these two trivially-copyable objects. Their
// retained pointers refer only to these objects/the canonical inline parts;
// there is no unmirrored mutable allocation in the plan's Matches relation.
// Caller MUST retain both exact objects under exclusive lifetime/mutation
// protection. Dynamic source/FD/team/clock/owner/device checks remain separate.
// Capturing after TWO complete original Matches calls,then comparing EVERY
// byte of BOTH objects,establishes unchanged validated DATA only. It is
// deliberately stricter for padding/unused bytes;it never accepts a changed
// object based on a digest,epoch,hint or caller-supplied ready boolean.
class PioneerQ38RecurrentPrefixPlanMirror {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38RecurrentPrefixPlan* plan=nullptr;
 unsigned char canonicalBytes[sizeof(PioneerQ38CanonicalModelPlan)];
 unsigned char planBytes[sizeof(PioneerQ38RecurrentPrefixPlan)];
 bool attempted=false,ready=false;
 static bool Overlap(const void* a,size_t bytes,const void* b,size_t size){
  const uintptr_t x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);
  return x>UINTPTR_MAX-bytes || y>UINTPTR_MAX-size || (x<y+size && y<x+bytes);
 }
 bool SameBytes(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& p)const{
  return canonical==&c && plan==&p && !memcmp(canonicalBytes,&c,sizeof(c)) && !memcmp(planBytes,&p,sizeof(p));
 }
public:
 PioneerQ38RecurrentPrefixPlanMirror()=default;
 PioneerQ38RecurrentPrefixPlanMirror(const PioneerQ38RecurrentPrefixPlanMirror&)=delete;
 PioneerQ38RecurrentPrefixPlanMirror& operator=(const PioneerQ38RecurrentPrefixPlanMirror&)=delete;
 bool Capture(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& p){
  if(attempted)return false;
  attempted=true;
  if(Overlap(&c,sizeof(c),this,sizeof(*this)) || Overlap(&p,sizeof(p),this,sizeof(*this))
   || Overlap(&c,sizeof(c),&p,sizeof(p)) || !c.Ready() || !p.Matches(c))return false;
  memcpy(canonicalBytes,&c,sizeof(c));memcpy(planBytes,&p,sizeof(p));canonical=&c;plan=&p;
  if(!p.Matches(c) || !SameBytes(c,p)){canonical=nullptr;plan=nullptr;return false;}
  ready=true;return true;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& p)const{return ready && SameBytes(c,p);}
};
static_assert(std::is_trivially_copyable<PioneerQ38CanonicalModelPlan>::value,"canonical geometry must stay fully inline/trivially copyable");
static_assert(std::is_trivially_copyable<PioneerQ38RecurrentPrefixPlan>::value,"prefix geometry must stay fully inline/trivially copyable");
static_assert(sizeof(PioneerQ38RecurrentPrefixPlanMirror)>3*1048576 && sizeof(PioneerQ38RecurrentPrefixPlanMirror)<7*1048576,
 "HEAP ONLY;this is not a kernel stack object or a driver authority");
#endif
