/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent ledger is isolated inspection only
#endif
#ifndef PIONEER_Q38_SMALL_RECURRENT_LEDGER_INSPECTION_H
#define PIONEER_Q38_SMALL_RECURRENT_LEDGER_INSPECTION_H
#include "PioneerQ38SmallRecurrentKernelLayoutInspection.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerResidentBanks.h"
#include <algorithm>
#include <vector>
// FNV checks consistency only. This is not authentication, readback or a
// live uploader. Sort by physical address because relocated parts append.
struct PioneerQ38SmallRecurrentLedgerInspection {
 struct Span {unsigned bank,first,count;};
 static bool Verify(const PioneerQ38CanonicalModelPlan& source,const PioneerQ38SmallRecurrentKernelLayoutInspection& layout,
  const uint8_t* ledger,const uint32_t* hashes,unsigned pages,uint64_t expectedDigest){
  if(!source.OriginalFormatLayout() || !layout.Ready() || source.Count()!=4678 || layout.Count()!=4678
   || !ledger || !hashes || (reinterpret_cast<uintptr_t>(hashes)&3) || pages!=PioneerResidentBanks::Pages)return false;
  constexpr unsigned perBank=unsigned(PioneerResidentBanks::Capacity/4096);
  std::vector<Span> spans;spans.reserve(3551);unsigned converted=0;
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);const auto* q=layout.At(i);if(!p || !q || q->original!=i || p->bank!=q->bank)return false;
   if(p->kind!=source.WeightKind)continue;
   if(q->bank>=7 || (q->offset&4095) || (q->allocated&4095) || !q->allocated)return false;
   const unsigned first=q->offset/4096,count=q->allocated/4096;
   if(first>=perBank || count>perBank-first)return false;
   spans.push_back({q->bank,first,count});if(q->encoding==layout.SmallRecurrentBfp8)converted++;
  }
  if(spans.size()!=3551 || converted!=96)return false;
  std::sort(spans.begin(),spans.end(),[](const Span& a,const Span& b){return a.bank<b.bank || (a.bank==b.bank && a.first<b.first);});
  uint64_t digest=UINT64_C(14695981039346656037),covered=0;unsigned end[7]={},batches=0;
  for(const auto& s:spans){
   if(s.first<end[s.bank])return false;const unsigned base=s.bank*perBank;
   for(unsigned page=end[s.bank];page<s.first;page++)if(ledger[base+page] || hashes[base+page])return false;
   for(unsigned n=0;n<s.count;n++){
    const unsigned index=base+s.first+n;if(ledger[index]!=1)return false;
    digest=(digest^index)*UINT64_C(1099511628211);digest=(digest^hashes[index])*UINT64_C(1099511628211);
   }
   end[s.bank]=s.first+s.count;covered+=s.count;batches+=(s.count+255)/256;
  }
  if(covered!=5067109 || batches!=21475 || digest!=expectedDigest)return false;
  for(unsigned bank=0;bank<7;bank++)for(unsigned page=end[bank];page<perBank;page++)
   if(ledger[bank*perBank+page] || hashes[bank*perBank+page])return false;
  return true;
 }
};
#endif
