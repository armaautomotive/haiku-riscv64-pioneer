/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent workspace is isolated offline geometry only
#endif
#ifndef PIONEER_Q38_SMALL_RECURRENT_WORKSPACE_H
#define PIONEER_Q38_SMALL_RECURRENT_WORKSPACE_H
#include "PioneerQ38HybridSmallRecurrentBfp8Plan.h"
#include "PioneerQ38RecurrentQ8MatrixWorkspace.h"
#include <array>

// Proposed ROOT records only; no upload or authority. Their range follows
// the large-recurrent records and precedes the existing session workspace.
struct PioneerQ38SmallRecurrentWorkspace {
 using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridSmallRecurrentBfp8Plan;
 enum : unsigned {Offset=0x3004a00,RecordWords=32,Records=96,Words=Records*RecordWords,Bytes=Words*4};
 static_assert(Offset==PioneerQ38RecurrentQ8MatrixWorkspace::Offset+PioneerQ38RecurrentQ8MatrixWorkspace::Bytes
  && Offset+Bytes<=PioneerQ38TokenLayout::SessionOffset && !(Offset&63),"small records must not overlap existing workspace");
 static bool Build(const Canonical& source,const Plan::Proposal& proposal,uint32_t* output,unsigned words){
  if(!output || words!=Words || (reinterpret_cast<uintptr_t>(output)&3)
   || proposal.parts.size()>SIZE_MAX/sizeof(proposal.parts[0]))return false;
  const uintptr_t dst=reinterpret_cast<uintptr_t>(output),src=reinterpret_cast<uintptr_t>(&source);
  const uintptr_t own=reinterpret_cast<uintptr_t>(&proposal),parts=reinterpret_cast<uintptr_t>(proposal.parts.data());
  const size_t partBytes=proposal.parts.size()*sizeof(proposal.parts[0]);
  if(dst>UINTPTR_MAX-Bytes || src>UINTPTR_MAX-sizeof(source) || own>UINTPTR_MAX-sizeof(proposal) || parts>UINTPTR_MAX-partBytes
   || (dst<src+sizeof(source) && src<dst+Bytes) || (dst<own+sizeof(proposal) && own<dst+Bytes)
   || (partBytes && dst<parts+partBytes && parts<dst+Bytes))return false;
  Plan::Proposal expected;if(!Plan::Build(source,expected) || expected.used!=proposal.used || expected.parts.size()!=proposal.parts.size())return false;
  for(unsigned i=0;i<expected.parts.size();i++){
   const auto& a=expected.parts[i];const auto& b=proposal.parts[i];
   if(a.original!=b.original || a.bank!=b.bank || a.offset!=b.offset || a.bytes!=b.bytes || a.allocated!=b.allocated || a.encoding!=b.encoding)return false;
  }
  std::array<uint32_t,Words> stage={};unsigned count=0;
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);if(!p)return false;
   if(p->kind!=Canonical::WeightKind || p->layer>=64 || p->layer%4==3 || (p->role!=10 && p->role!=11))continue;
   const auto& q=expected.parts[i];const unsigned record=(p->layer-p->layer/4)*2+p->role-10;
   if(record>=Records || q.encoding!=Plan::SmallRecurrentBfp8 || q.bank!=p->bank || q.bank>=7
    || q.offset>UINT32_MAX-Canonical::Base || (q.offset&4095) || q.bytes!=Plan::Bytes || q.allocated!=Plan::Bytes)return false;
   const unsigned at=record*RecordWords+q.bank*3;if(stage[at])return false;
   stage[at]=Canonical::Base+q.offset;stage[at+2]=64;count++;
  }
  if(count!=Records)return false;
  for(unsigned i=0;i<Words;i++)output[i]=stage[i];return true;
 }
};
#endif
