/* SPDX-License-Identifier: MIT */
// Full expected ledger and deliberate holes, synthetic bytes only.
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#define PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION 1
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"
#include "../PioneerQ38DenseQ8LedgerInspection.h"
#include <vector>
int main(){
 using Layout=PioneerQ38DenseQ8KernelLayoutInspection;using Check=PioneerQ38DenseQ8LedgerInspection;
 auto source=std::make_unique<PioneerQ38CanonicalModelPlan>();auto parent=std::make_unique<Layout::Parent>();auto layout=std::make_unique<Layout>();
 assert(source->Build() && parent->Build(*source) && layout->Build(*source,*parent));
 const unsigned pages=PioneerResidentBanks::Pages,perBank=unsigned(PioneerResidentBanks::Capacity/4096);
 std::vector<uint8_t> ledger(pages,0);std::vector<uint32_t> hashes(pages,0);uint64_t digest=UINT64_C(14695981039346656037);
 unsigned recorded=0,hole=UINT32_MAX,first=UINT32_MAX;
 for(unsigned i=0;i<source->Count();i++){
  const auto* p=source->Part(i);const auto* q=layout->At(i);const auto* old=parent->At(i);assert(p && q && old);
  if(p->kind!=source->WeightKind)continue;
  const unsigned start=q->bank*perBank+q->offset/4096;
  for(unsigned n=0;n<q->allocated/4096;n++){
   const unsigned index=start+n;assert(!ledger[index]);ledger[index]=1;hashes[index]=index*2654435761U+17;
   digest=(digest^index)*UINT64_C(1099511628211);digest=(digest^hashes[index])*UINT64_C(1099511628211);
   if(first==UINT32_MAX)first=index;recorded++;
  }
  if(q->encoding==Layout::DenseQ8Bfp8 && q->allocated<old->allocated && hole==UINT32_MAX)hole=start+q->allocated/4096;
 }
 assert(recorded==5149957 && first!=UINT32_MAX && hole!=UINT32_MAX && !ledger[hole]);
 assert(Check::Verify(*source,*layout,ledger.data(),hashes.data(),pages,digest));
 assert(!Check::Verify(*source,*layout,ledger.data(),hashes.data(),pages,digest^1));
 ledger[hole]=1;assert(!Check::Verify(*source,*layout,ledger.data(),hashes.data(),pages,digest));ledger[hole]=0;
 hashes[hole]=1;assert(!Check::Verify(*source,*layout,ledger.data(),hashes.data(),pages,digest));hashes[hole]=0;
 ledger[first]=0;assert(!Check::Verify(*source,*layout,ledger.data(),hashes.data(),pages,digest));ledger[first]=1;
 hashes[first]^=1;assert(!Check::Verify(*source,*layout,ledger.data(),hashes.data(),pages,digest));
 puts("PASS complete dense-Q8 synthetic ledger, holes and altered page/digest refusal; NO DEVICE OR MODEL AUTHENTICATION");
}
