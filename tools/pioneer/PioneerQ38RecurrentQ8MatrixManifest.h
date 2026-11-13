/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_Q8_MATRIX_MANIFEST_H
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_MANIFEST_H
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent Q8 matrix manifest is offline geometry only
#endif
#include "PioneerQ38HybridRecurrentQ8MatrixPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <array>

// Canonical metadata only, not proof of upload, ownership or device lifetime.
struct PioneerQ38RecurrentQ8MatrixManifest {
 using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridRecurrentQ8MatrixPlan;
 enum : unsigned {Fields=3,Banks=7,Roles=3,Layers=48,Words=Layers*Roles*Banks*Fields};
 struct Scratch {std::array<uint32_t,Words> stage={};};
 static unsigned Index(unsigned layer,unsigned role,unsigned bank){return ((layer*Roles+role)*Banks+bank)*Fields;}
 static bool BuildWithScratch(const Canonical& source,const Plan::Proposal& proposal,uint32_t* output,unsigned words,Scratch& scratch){
  if(!output || words!=Words || (reinterpret_cast<uintptr_t>(output)&3) || proposal.parts.size()>SIZE_MAX/sizeof(proposal.parts[0]))return false;
  const uintptr_t dst=reinterpret_cast<uintptr_t>(output),src=reinterpret_cast<uintptr_t>(&source);
  const uintptr_t own=reinterpret_cast<uintptr_t>(&proposal),parts=reinterpret_cast<uintptr_t>(proposal.parts.data());
  const uintptr_t work=reinterpret_cast<uintptr_t>(&scratch);
  const size_t bytes=Words*sizeof(uint32_t),partBytes=proposal.parts.size()*sizeof(proposal.parts[0]);
  if(dst>UINTPTR_MAX-bytes || src>UINTPTR_MAX-sizeof(source) || own>UINTPTR_MAX-sizeof(proposal) || parts>UINTPTR_MAX-partBytes || work>UINTPTR_MAX-sizeof(scratch)
   || (dst<src+sizeof(source) && src<dst+bytes) || (dst<own+sizeof(proposal) && own<dst+bytes)
   || (partBytes && dst<parts+partBytes && parts<dst+bytes)
   || (dst<work+sizeof(scratch) && work<dst+bytes) || (work<src+sizeof(source) && src<work+sizeof(scratch))
   || (work<own+sizeof(proposal) && own<work+sizeof(scratch))
   || (partBytes && work<parts+partBytes && parts<work+sizeof(scratch)))return false;
  Plan::Proposal expected;if(!Plan::Build(source,expected) || expected.used!=proposal.used || expected.parts.size()!=proposal.parts.size())return false;
  for(unsigned i=0;i<expected.parts.size();i++){
   const auto& a=expected.parts[i];const auto& b=proposal.parts[i];
   if(a.original!=b.original || a.bank!=b.bank || a.offset!=b.offset || a.bytes!=b.bytes || a.allocated!=b.allocated || a.encoding!=b.encoding)return false;
  }
  auto& stage=scratch.stage;stage.fill(0);unsigned count=0;
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);if(!p)return false;
   if(p->kind!=Canonical::WeightKind || p->layer>=64 || p->layer%4==3
    || (p->role!=5 && p->role!=6 && p->role!=13))continue;
   const auto& q=expected.parts[i];if(q.encoding!=Plan::RecurrentQ8Bfp8 || q.bank!=p->bank || q.bank>=7
    || q.offset>UINT32_MAX-Canonical::Base || (q.offset&4095))return false;
   const unsigned ordinal=p->layer-p->layer/4,role=p->role==5?0:p->role==6?1:2;
   if(ordinal>=Layers)return false;
   const unsigned at=Index(ordinal,role,p->bank);if(stage[at] || !p->rows)return false;
   stage[at]=Canonical::Base+q.offset;stage[at+1]=p->firstRow;stage[at+2]=p->rows;count++;
  }
  if(count!=Layers*Roles*Banks)return false;
  for(unsigned i=0;i<Words;i+=Fields)if(!stage[i] || !stage[i+2])return false;
  for(unsigned i=0;i<Words;i++)output[i]=stage[i];return true;
 }
 static bool Build(const Canonical& source,const Plan::Proposal& proposal,uint32_t* output,unsigned words){
  Scratch scratch;return BuildWithScratch(source,proposal,output,words,scratch);
 }
};
#endif
