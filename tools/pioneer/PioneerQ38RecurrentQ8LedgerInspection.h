/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_Q8_LEDGER_INSPECTION_H
#define PIONEER_Q38_RECURRENT_Q8_LEDGER_INSPECTION_H
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent Q8 ledger is isolated inspection only
#endif
#include "PioneerQ38RecurrentQ8KernelLayoutInspection.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerResidentBanks.h"
// FNV is a consistency check only: not authentication, readback or authority.
struct PioneerQ38RecurrentQ8LedgerInspection {
 static bool Verify(const PioneerQ38CanonicalModelPlan& source,const PioneerQ38RecurrentQ8KernelLayoutInspection& layout,
  const uint8_t* ledger,const uint32_t* hashes,unsigned pages,uint64_t expectedDigest){
  if(!source.OriginalFormatLayout() || !layout.Ready() || source.Count()!=4678 || layout.Count()!=4678
   || !ledger || !hashes || (reinterpret_cast<uintptr_t>(hashes)&3) || pages!=PioneerResidentBanks::Pages)return false;
  constexpr unsigned perBank=unsigned(PioneerResidentBanks::Capacity/4096);
  uint64_t digest=UINT64_C(14695981039346656037),bytes=0;unsigned end[7]={},parts=0,covered=0,spans=0;
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);const auto* q=layout.At(i);if(!p || !q || q->original!=i || p->bank!=q->bank)return false;
   if(p->kind!=source.WeightKind)continue;
   if(q->bank>=7 || (q->offset&4095) || (q->allocated&4095) || !q->allocated)return false;
   const unsigned first=q->offset/4096,count=q->allocated/4096;
   if(first<end[q->bank] || first>=perBank || count>perBank-first)return false;
   const unsigned base=q->bank*perBank;
   for(unsigned hole=end[q->bank];hole<first;hole++)if(ledger[base+hole] || hashes[base+hole])return false;
   for(unsigned n=0;n<count;n++){
    const unsigned index=base+first+n;if(ledger[index]!=1)return false;
    digest=(digest^index)*UINT64_C(1099511628211);digest=(digest^hashes[index])*UINT64_C(1099511628211);
   }
   end[q->bank]=first+count;bytes+=uint64_t(count)*4096;covered+=count;spans+=(count+255)/256;parts++;
  }
  if(parts!=3551 || covered!=5065477 || spans!=21475 || bytes!=UINT64_C(20748193792) || digest!=expectedDigest)return false;
  for(unsigned bank=0;bank<7;bank++)for(unsigned page=end[bank];page<perBank;page++)
   if(ledger[bank*perBank+page] || hashes[bank*perBank+page])return false;
  return true;
 }
};
#endif
