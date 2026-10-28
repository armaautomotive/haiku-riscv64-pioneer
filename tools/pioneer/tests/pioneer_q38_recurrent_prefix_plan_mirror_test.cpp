/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_RECURRENT_PREFIX_PLAN_MIRROR_PROTOTYPE 1
#include "../PioneerQ38RecurrentPrefixPlanMirror.h"
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <memory>
static void Barrier(){asm volatile("" ::: "memory");}
int main(){
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();
 auto cold=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build() && other->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 auto p=std::make_unique<PioneerQ38RecurrentPrefixPlan>();auto q=std::make_unique<PioneerQ38RecurrentPrefixPlan>();
 auto failed=std::make_unique<PioneerQ38RecurrentPrefixPlanMirror>();
 assert(!failed->Matches(*c,*p) && !failed->Capture(*cold,*p));
 assert(p->Build(*c,0,{1,2,3,4},sources.data()) && q->Build(*other,0,{1,2,3,4},sources.data()));
 assert(!failed->Capture(*c,*p) && !failed->Matches(*c,*p));
 auto mirror=std::make_unique<PioneerQ38RecurrentPrefixPlanMirror>();
 assert(mirror->Capture(*c,*p) && !mirror->Capture(*c,*p) && mirror->Matches(*c,*p)
  && !mirror->Matches(*other,*p) && !mirror->Matches(*c,*q) && !mirror->Matches(*other,*q));
 unsigned mutations=0;
 auto mutate=[&](void* location){auto* byte=static_cast<unsigned char*>(location);*byte^=1;
  assert(!mirror->Matches(*c,*p));*byte^=1;assert(mirror->Matches(*c,*p));mutations++;};
 // Every byte of all live canonical part records,including geometry not
 // used by this prefix. Exact mirrors intentionally reject unused changes.
 for(unsigned i=0;i<c->Count();i++)for(unsigned byte=0;byte<sizeof(PioneerQ38CanonicalPart);byte++)
  mutate(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(c->Part(i)))+byte);
 // Every serialized word across ALL3 layers,not merely packet headers.
 namespace L=PioneerQ38MixedRecurrentCohortLayout;
 for(unsigned layer=0;layer<3;layer++){
  const auto& plan=*p->Layer(layer);
  for(unsigned i=0;i<128;i++)mutate(const_cast<uint32_t*>(plan.Outer())+i);
  for(unsigned slot=0;slot<8;slot++)for(unsigned i=0;i<1280;i++)mutate(const_cast<uint32_t*>(plan.Recipe(slot))+i);
  for(unsigned i=0;i<L::ConvWords;i++)mutate(const_cast<uint32_t*>(plan.Conv())+i);
  for(unsigned i=0;i<L::GdnWords;i++)mutate(const_cast<uint32_t*>(plan.Gdn())+i);
 }
 for(unsigned worker=0;worker<120;worker++)for(unsigned i=0;i<64;i++)mutate(const_cast<uint32_t*>(p->Boot(worker))+i);
 // Also cover representation pages/private pointer bytes without ever
 // dereferencing a deliberately mutated pointer. Restore before real Matches.
 for(unsigned at=0;at<sizeof(*c);at+=4096)mutate(reinterpret_cast<unsigned char*>(c.get())+at);
 for(unsigned at=0;at<sizeof(*p);at+=4096)mutate(reinterpret_cast<unsigned char*>(p.get())+at);
 assert(p->Matches(*c) && q->Matches(*other));
 for(unsigned bank=1;bank<7;bank++){
  auto plan=std::make_unique<PioneerQ38RecurrentPrefixPlan>();auto copy=std::make_unique<PioneerQ38RecurrentPrefixPlanMirror>();
  assert(plan->Build(*c,bank,{1,2,3,4},sources.data()) && copy->Capture(*c,*plan) && copy->Matches(*c,*plan));
 }
 constexpr unsigned slowCount=3,mirrorCount=128;unsigned observed=0;
 auto begin=std::chrono::steady_clock::now();
 for(unsigned i=0;i<slowCount;i++){Barrier();observed+=p->Matches(*c);}
 auto middle=std::chrono::steady_clock::now();
 for(unsigned i=0;i<mirrorCount;i++){Barrier();observed+=mirror->Matches(*c,*p);}
 auto end=std::chrono::steady_clock::now();assert(observed==slowCount+mirrorCount);
 printf("PASS HOST-ONLY prefix exact-data mirror/7banks/%u reached byte-or-word mutations/ALL3 serialized metadata+120boots/canonical inline parts+private pages/foreign-pointer+one-shot failure refusals;EVERY compare checks BOTH full objects,NO digest/ready shortcut/native or IO grant\n",mutations);
 printf("HOST_DIAGNOSTIC_TIMING originalMatches%u/totalNs%lld exactMirror%u/totalNs%lld canonicalBytes%zu planBytes%zu;LOCAL CPU experiment ONLY,NOT driver/inference/physical or tokens-per-second evidence\n",
  slowCount,(long long)std::chrono::duration_cast<std::chrono::nanoseconds>(middle-begin).count(),mirrorCount,
  (long long)std::chrono::duration_cast<std::chrono::nanoseconds>(end-middle).count(),sizeof(*c),sizeof(*p));
}
