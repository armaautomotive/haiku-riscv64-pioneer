/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38RecurrentKernelCompileRole.h"
#if !defined(PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION != 1 \
 || ((defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent native binding is isolated inspection only
#endif
#ifndef PIONEER_Q38_SMALL_RECURRENT_NATIVE_BINDING_H
#define PIONEER_Q38_SMALL_RECURRENT_NATIVE_BINDING_H
#include "../../../../../tools/pioneer/PioneerQ38SmallRecurrentUploadReceiptInspection.h"
// Owner/geometry bridge only. It does not authenticate source data, bind a
// native journal, write the workspace or authorize accelerator execution.
struct PioneerQ38SmallRecurrentNativeBinding {
 template<class Context,class Cookie>static bool Matches(Context& c,const Cookie* fd,uint64_t generation){
  return fd && fd->magic==0x50313030 && fd->context==&c && fd->team>=0 && generation
   && !c.q38HybridReceipt && !c.q38Qgw8Receipt && !c.q38QgwdReceipt && c.q38SmallRecurrentReceipt && c.q38Span
   && c.q38SmallRecurrentReceipt->MatchesLease(c.q38Span)
   && c.q38SmallRecurrentReceipt->MatchesBanks(&c.banks,&c.baseline.baseline.baseline.baseline.dram)
   && c.q38SmallRecurrentReceipt->Matches(&c,fd,c.hashes,c.initialized,fd->team,generation);
 }
 template<class Context,class Cookie>static bool DeriveLogical(Context& c,const Cookie* fd,uint64_t generation,
  PioneerQ38CanonicalModelPlan& logical){
  return Matches(c,fd,generation)
   && c.q38SmallRecurrentReceipt->DeriveLogical(logical,&c,fd,c.hashes,c.initialized,fd->team,generation)
   && Matches(c,fd,generation)
   && c.q38SmallRecurrentReceipt->MatchesLogical(logical,&c,fd,c.hashes,c.initialized,fd->team,generation);
 }
 template<class Context,class Cookie,class Cohort,class Journal>
 static bool BindJournal(Context& c,const Cookie* fd,uint64_t generation,Cohort* cohort,Journal& journal){
  return cohort && Matches(c,fd,generation)
   && journal.BindDenseReceiptInspection(*c.q38SmallRecurrentReceipt,&c,cohort,fd,c.hashes,fd->team,generation);
 }
};
#endif
