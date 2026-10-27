/* SPDX-License-Identifier: MIT */
// TEST-only differential geometry/host-metadata benchmark. NO native IO.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PioneerQ38MixedLiveBlockPlan BaselineMixedLiveBlockPlan
#include "reference/pioneer_q38_mixed_live_block_plan_before_lookup.h"
#undef PioneerQ38MixedLiveBlockPlan
#undef PIONEER_Q38_MIXED_LIVE_BLOCK_PLAN_H
#include "../PioneerQ38MixedLiveBlockPlan.h"
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <memory>
using C=PioneerQ38CanonicalModelPlan;
using Old=BaselineMixedLiveBlockPlan;using New=PioneerQ38MixedLiveBlockPlan;
static_assert(sizeof(Old)==sizeof(New),"NO plan layout/authority change");
static uint64_t payloadHash=14695981039346656037ULL;static unsigned words=0;
static void Same(const uint32_t* a,const uint32_t* b,unsigned n){
 assert(a && b);for(unsigned i=0;i<n;i++){assert(a[i]==b[i]);payloadHash^=a[i];payloadHash*=1099511628211ULL;words++;}
}
template<class P> static uint64_t Measure(const P& p,const C& c){
 auto at=std::chrono::steady_clock::now();volatile unsigned passed=0;
 for(unsigned i=0;i<100;i++){std::atomic_signal_fence(std::memory_order_seq_cst);passed+=unsigned(p.Matches(c));}
 assert(passed==100);return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-at).count();
}
int main(){
 auto c=std::make_unique<C>();assert(c->Build());std::array<uint32_t,1920> sources;
 for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 unsigned plans=0,mutations=0,accepted=0,refused=0;uint64_t oldUs=0,newUs=0;
 for(unsigned layer=3;layer<64;layer+=4)for(unsigned bank=0;bank<7;bank++)for(unsigned position:{0U,8191U}){
  auto a=std::make_unique<Old>();auto b=std::make_unique<New>();
  unsigned input=c->Base+c->Workspace(bank)->offset+PioneerQ38MixedLiveBlockRootLayout::FinalOffset;
  assert(a->Build(*c,layer,position,248319,bank,7000,1,{1,2,3,4},sources.data(),bank,input)
   && b->Build(*c,layer,position,248319,bank,7000,1,{1,2,3,4},sources.data(),bank,input));
  assert(a->Matches(*c) && b->Matches(*c));plans++;
  Same(a->Outer(),b->Outer(),128);for(unsigned w=0;w<120;w++)Same(a->Boot(w),b->Boot(w),64);
  for(unsigned slot=0;slot<7;slot++)Same(a->Recipe(slot),b->Recipe(slot),1280);
  for(unsigned w=0;w<96;w++)Same(a->Dense(w),b->Dense(w),64);
  if(layer!=3 || bank || position)continue;
  auto check=[&]{bool before=a->Matches(*c),after=b->Matches(*c);assert(before==after);mutations++;if(before)accepted++;else refused++;};
  uint32_t PioneerQ38CanonicalPart::* fields[]={&PioneerQ38CanonicalPart::kind,&PioneerQ38CanonicalPart::layer,&PioneerQ38CanonicalPart::role,
   &PioneerQ38CanonicalPart::type,&PioneerQ38CanonicalPart::bank,&PioneerQ38CanonicalPart::firstRow,&PioneerQ38CanonicalPart::rows,
   &PioneerQ38CanonicalPart::rowBytes,&PioneerQ38CanonicalPart::offset,&PioneerQ38CanonicalPart::bytes,&PioneerQ38CanonicalPart::allocated};
  const PioneerQ38CanonicalPart* duplicate=nullptr;
  for(unsigned i=0;i<c->Count();i++){
   auto* p=const_cast<PioneerQ38CanonicalPart*>(c->Part(i));if(p->kind!=c->KvKind || p->layer!=layer)continue;
   if(!duplicate)duplicate=p;
   for(auto member:fields)for(unsigned bit=0;bit<32;bit++){p->*member^=uint32_t(1)<<bit;check();p->*member^=uint32_t(1)<<bit;}
  }
  assert(duplicate);auto* extra=const_cast<PioneerQ38CanonicalPart*>(c->Part(0));const auto saved=*extra;*extra=*duplicate;check();
  assert(!a->Matches(*c) && !b->Matches(*c));*extra=saved;
  assert(a->Matches(*c) && b->Matches(*c));oldUs=Measure(*a,*c);newUs=Measure(*b,*c);
 }
 printf("PASS DATA ONLY differential plans%u/words%u/hash%016llx/ALL16dense layers7banks/context endpoints;KV all11fieldsx32bits+duplicate mutations%u/same legacy accepts%u/refusals%u;NO guard removed/persistent cache/native IO/grant/card TPS\n",
  plans,words,(unsigned long long)payloadHash,mutations,accepted,refused);
 printf("HOST_METADATA_ONLY100Matches baseline_us%llu/candidate_us%llu;NOT silicon/inference rate\n",(unsigned long long)oldUs,(unsigned long long)newUs);
}
