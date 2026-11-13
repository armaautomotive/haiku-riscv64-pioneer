/* SPDX-License-Identifier: MIT */
#include "PioneerQ38RecurrentKernelCompileRole.h"
#if !defined(PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION != 1 \
 || ((defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent ledger is isolated inspection only
#endif
#ifndef PIONEER_Q38_SMALL_RECURRENT_LEDGER_INSPECTION_H
#define PIONEER_Q38_SMALL_RECURRENT_LEDGER_INSPECTION_H
#include "PioneerQ38SmallRecurrentKernelLayoutInspection.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerResidentBanks.h"
// FNV checks consistency only. This is not authentication, readback or a
// live uploader. Each bank contains the original ordered spans followed by
// relocated small spans in original order, so no heap sort is needed.
struct PioneerQ38SmallRecurrentLedgerInspection {
 static bool Verify(const PioneerQ38CanonicalModelPlan& source,const PioneerQ38SmallRecurrentKernelLayoutInspection& layout,
  const uint8_t* ledger,const uint32_t* hashes,unsigned pages,uint64_t expectedDigest){
  if(!source.OriginalFormatLayout() || !layout.Ready() || source.Count()!=4678 || layout.Count()!=4678
   || !ledger || !hashes || (reinterpret_cast<uintptr_t>(hashes)&3) || pages!=PioneerResidentBanks::Pages)return false;
  constexpr unsigned perBank=unsigned(PioneerResidentBanks::Capacity/4096);
  unsigned converted=0,parts=0,batches=0;uint64_t covered=0;
  uint64_t digest=UINT64_C(14695981039346656037);
  unsigned end[7]={};
  for(unsigned bank=0;bank<7;bank++)for(unsigned relocated=0;relocated<2;relocated++)
   for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);const auto* q=layout.At(i);if(!p || !q || q->original!=i || p->bank!=q->bank)return false;
   if(p->kind!=source.WeightKind)continue;
   if(q->bank>=7 || (q->offset&4095) || (q->allocated&4095) || !q->allocated)return false;
   if(q->bank!=bank || unsigned(q->encoding==layout.SmallRecurrentBfp8)!=relocated)continue;
   const unsigned first=q->offset/4096,count=q->allocated/4096;
   if(first<end[bank] || first>=perBank || count>perBank-first)return false;
   const unsigned base=bank*perBank;
   for(unsigned page=end[bank];page<first;page++)if(ledger[base+page] || hashes[base+page])return false;
   for(unsigned n=0;n<count;n++){
    const unsigned index=base+first+n;if(ledger[index]!=1)return false;
    digest=(digest^index)*UINT64_C(1099511628211);digest=(digest^hashes[index])*UINT64_C(1099511628211);
   }
   end[bank]=first+count;covered+=count;batches+=(count+255)/256;parts++;converted+=relocated;
  }
  if(parts!=3551 || converted!=96 || covered!=5067109 || batches!=21475 || digest!=expectedDigest)return false;
  for(unsigned bank=0;bank<7;bank++)for(unsigned page=end[bank];page<perBank;page++)
   if(ledger[bank*perBank+page] || hashes[bank*perBank+page])return false;
  return true;
 }
};
#endif
