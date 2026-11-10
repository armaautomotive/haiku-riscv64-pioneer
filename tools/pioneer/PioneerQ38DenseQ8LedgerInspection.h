/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QgwdNativeInspectionRole.h"
#ifndef PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION_H
#define PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION_H
#if !defined(PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION) || PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION != 1 \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION) && (!defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1)) \
 || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Dense Q8 ledger is isolated inspection only
#endif
#include "PioneerQ38DenseQ8KernelLayoutInspection.h"

// Offline-only expected upload-ledger verification. The page digest is an
// ordered FNV consistency check, NOT a cryptographic model hash, device
// readback, an ownership receipt, source authentication or launch grant.
// Unlike QGW8, this layout intentionally leaves uninitialized holes inside
// learned banks, so a contiguous-prefix ledger check is incorrect.
struct PioneerQ38DenseQ8LedgerInspection {
 static bool Verify(const PioneerQ38CanonicalModelPlan& source,const PioneerQ38DenseQ8KernelLayoutInspection& layout,
  const uint8_t* ledger,const uint32_t* hashes,unsigned pages,uint64_t expectedDigest){
  if(!source.OriginalFormatLayout() || !layout.Ready() || source.Count()!=4678 || layout.Count()!=4678
   || !ledger || !hashes || (reinterpret_cast<uintptr_t>(hashes)&3) || pages!=PioneerResidentBanks::Pages)return false;
  constexpr unsigned perBank=unsigned(PioneerResidentBanks::Capacity/4096);
  uint64_t digest=UINT64_C(14695981039346656037),bytes=0;unsigned end[7]={},parts=0,spans=0,covered=0;
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
  if(parts!=3551 || spans!=21907 || covered!=5149957 || bytes!=UINT64_C(21094223872) || digest!=expectedDigest)return false;
  for(unsigned bank=0;bank<7;bank++)for(unsigned page=end[bank];page<perBank;page++)
   if(ledger[bank*perBank+page] || hashes[bank*perBank+page])return false;
  return true;
 }
};
#endif
