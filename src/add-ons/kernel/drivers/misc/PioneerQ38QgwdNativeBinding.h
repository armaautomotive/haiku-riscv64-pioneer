/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38QgwdNativeInspectionRole.h"
#ifndef PIONEER_Q38_QGWD_NATIVE_BINDING_H
#define PIONEER_Q38_QGWD_NATIVE_BINDING_H
#if !defined(PIONEER_Q38_QGWD_CONTROL_INSPECTION) || PIONEER_Q38_QGWD_CONTROL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION) || PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION) || PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION != 1 \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION) && (!defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1)) \
 || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGWD native-binding bridge is isolated inspection only
#endif
#include "../../../../../tools/pioneer/PioneerQ38DenseQ8UploadReceiptInspection.h"

// Exact driver context, cookie, span, owner and generation bridge; the
// logical layout is geometry only. Neither method authenticates source bytes,
// reads device memory, initializes state, or grants native execution.
struct PioneerQ38QgwdNativeBinding {
 template<class Context,class Cookie>static bool Matches(Context& c,const Cookie* fd,uint64_t generation){
  return fd && fd->magic==0x50313030 && fd->context==&c && fd->team>=0 && generation
   && !c.q38HybridReceipt && !c.q38Qgw8Receipt && c.q38QgwdReceipt && c.q38Span
   && c.q38QgwdReceipt->MatchesLease(c.q38Span)
   && c.q38QgwdReceipt->Matches(&c,fd,c.hashes,c.initialized,fd->team,generation);
 }
 template<class Context,class Cookie>static bool DeriveLogical(Context& c,const Cookie* fd,uint64_t generation,
  PioneerQ38CanonicalModelPlan& logical){
  if(!Matches(c,fd,generation) || !c.q38QgwdReceipt->Revalidate(&c,fd,c.hashes,c.initialized,fd->team,generation)
   || !c.q38QgwdReceipt->DeriveCanonical(logical,false,&c,fd,c.hashes,c.initialized,fd->team,generation))return false;
  return Matches(c,fd,generation) && c.q38QgwdReceipt->MatchesLogical(logical,&c,fd,c.hashes,c.initialized,
   fd->team,generation,c.banks,c.baseline.baseline.baseline.baseline.dram);
 }
 template<class Context,class Cookie,class Cohort,class Journal>
 static bool BindJournal(Context& c,const Cookie* fd,uint64_t generation,Cohort* cohort,Journal& journal){
  return cohort && Matches(c,fd,generation)
   && journal.BindDenseReceiptInspection(*c.q38QgwdReceipt,&c,cohort,fd,c.hashes,fd->team,generation);
 }
};
#endif
