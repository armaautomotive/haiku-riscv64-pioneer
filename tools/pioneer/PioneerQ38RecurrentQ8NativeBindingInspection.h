/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_RECURRENT_Q8_CANONICAL_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_CANONICAL_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent native binding is offline inspection only
#endif
#ifndef PIONEER_Q38_RECURRENT_Q8_NATIVE_BINDING_INSPECTION_H
#define PIONEER_Q38_RECURRENT_Q8_NATIVE_BINDING_INSPECTION_H
#include "PioneerQ38RecurrentQ8UploadReceiptInspection.h"

// Fake-owner binding only. This checks identity and the retained receipt;
// no actual FD lock, source authentication, MMIO, device readback or RUN.
struct PioneerQ38RecurrentQ8NativeBindingInspection {
 template<class Context,class Cookie>static bool Matches(Context& c,const Cookie* fd,uint64_t generation){
  return fd && fd->magic==0x50313030 && fd->context==&c && fd->team>=0 && generation
   && !c.hybridReceipt && !c.qgw8Receipt && !c.denseReceipt && c.recurrentReceipt && c.span && c.banks && c.proof
   && c.recurrentReceipt->MatchesLease(c.span) && c.recurrentReceipt->MatchesBanks(c.banks,c.proof)
   && c.recurrentReceipt->Matches(&c,fd,c.hashes,c.initialized,fd->team,generation);
 }
 template<class Context,class Cookie>static bool Derive(Context& c,const Cookie* fd,uint64_t generation,
  PioneerQ38CanonicalModelPlan& output,bool physical){
  if(!Matches(c,fd,generation) || !c.recurrentReceipt->DeriveCanonical(output,physical,&c,fd,c.hashes,c.initialized,fd->team,generation))return false;
  return Matches(c,fd,generation) && (physical ? output.RecurrentQ8InspectionLayout() :
   c.recurrentReceipt->MatchesLogical(output,&c,fd,c.hashes,c.initialized,fd->team,generation));
 }
};
#endif
