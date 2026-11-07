/* SPDX-License-Identifier: MIT */
// Compare the freestanding QGW8 layout with the independently built host
// proposal. Synthetic geometry only; no device, ledger or inference.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8KernelLayout.h"
#include "../PioneerQ38HybridHeadModelPlan.h"
#include <memory>

int main(){
 auto source=std::make_unique<PioneerQ38CanonicalModelPlan>();
 auto invalid=std::make_unique<PioneerQ38Qgw8KernelLayout>();
 assert(!invalid->Build(*source) && !invalid->Ready() && !invalid->Count() && !invalid->At(0));
 assert(source->Build());
 assert(!invalid->Build(*source)); // one-shot, even after the source becomes ready
 auto old=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(old->BuildHybridDiagnostic());
 auto refused=std::make_unique<PioneerQ38Qgw8KernelLayout>();assert(!refused->Build(*old) && !refused->Ready());
 auto layout=std::make_unique<PioneerQ38Qgw8KernelLayout>();assert(layout->Build(*source) && layout->Ready());
 assert(!layout->Build(*source));
 PioneerQ38HybridHeadModelPlan::Proposal host;assert(PioneerQ38HybridHeadModelPlan::Build(*source,host));
 assert(host.parts.size()==layout->Count() && layout->Count()==4678 && !layout->At(4678));
 uint64_t next[7]={};unsigned four=0,eight=0,head=0,original=0;
 for(unsigned i=0;i<layout->Count();i++){
  const auto* a=layout->At(i);const auto& b=host.parts[i];assert(a && a->original==i && a->bank<7);
  assert(a->original==b.original && a->bank==b.bank && a->offset==b.offset && a->bytes==b.bytes
   && a->allocated==b.allocated && a->encoding==b.encoding && a->offset==next[a->bank]);
  next[a->bank]+=a->allocated;
  if(a->encoding==layout->Original)original++;
  else if(a->encoding==layout->Bfp4)four++;
  else if(a->encoding==layout->Bfp8)eight++;
  else {assert(a->encoding==layout->HeadBfp8);head++;}
 }
 for(unsigned b=0;b<7;b++)assert(next[b]==layout->Used(b) && next[b]==host.used[b]);
 assert(original==3327 && four==1113 && eight==231 && head==7 && !layout->Used(7));
 puts("PASS freestanding QGW8 kernel geometry matches all 4678 independent host parts and seven banks; distinct formats, no receipt/device/Work");
}
