/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_HEAD_MODEL_PLAN_H
#define PIONEER_Q38_HYBRID_HEAD_MODEL_PLAN_H
#if !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1
#error QGW8 head repack is isolated offline plan DATA only
#endif
#include "PioneerQ38HybridHeadWorkerLayout.h"
#include "PioneerQ38HybridModelPlan.h"

// Repack the SAME bank assignments in canonical order; this is an OFFLINE
// proposal only. It must never be interpreted as allocation, initialized
// bytes, original Q6 authority, native owner or a GPU address capability.
struct PioneerQ38HybridHeadModelPlan {
 using Base=PioneerQ38HybridModelPlan;
 enum : unsigned {HeadBfp8=9};
 struct Proposal {std::vector<Base::Part> parts;std::array<uint64_t,7> used{};};
 template<class Canonical>static bool Build(const Canonical& source,Proposal& output){
  Base::Proposal reference;
  if(!Base::Build(source,UINT64_C(0x1ffc000000000000),reference) || reference.parts.size()!=source.Count())return false;
  Proposal staged;staged.parts.reserve(source.Count());unsigned headCount=0,rows=0,banks=0;
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);const auto& q=reference.parts[i];
   if(!p || q.original!=i || q.bank!=p->bank || q.bank>=7 || !q.bytes || q.bytes>q.allocated || (q.allocated&4095))return false;
   uint64_t bytes=q.bytes,allocated=q.allocated;unsigned encoding=q.encoding;
   if(p->kind==Canonical::WeightKind && p->layer==64 && p->role==1){
    if(p->type!=14 || p->firstRow!=rows || !p->rows || (p->firstRow&63) || (p->rows&63)
     || p->rowBytes!=4240 || uint64_t(p->rows)*4240!=p->bytes || q.encoding!=Base::Original
     || (banks&(1U<<p->bank)))return false;
    bytes=uint64_t(p->rows)*5440;allocated=(bytes+4095)&~uint64_t(4095);
    rows+=p->rows;banks|=1U<<p->bank;headCount++;encoding=HeadBfp8;
   }
   auto& at=staged.used[p->bank];
   if(!bytes || bytes>UINT32_MAX || allocated>UINT32_MAX || at>Canonical::Capacity || allocated>Canonical::Capacity-at)return false;
   staged.parts.push_back({i,p->bank,uint32_t(at),uint32_t(bytes),uint32_t(allocated),encoding});at+=allocated;
  }
  if(headCount!=7 || rows!=248320 || banks!=127 || staged.parts.size()!=source.Count())return false;
  output=std::move(staged);return true;
 }
};
#endif
