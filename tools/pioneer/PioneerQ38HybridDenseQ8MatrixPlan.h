/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_DENSE_Q8_MATRIX_PLAN_H
#define PIONEER_Q38_HYBRID_DENSE_Q8_MATRIX_PLAN_H
#if !defined(PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Dense Q8 matrix plan is offline geometry only
#endif
#include "PioneerQ38HybridHeadModelPlan.h"

// Convert only dense-layer Q/G, K and V stripes to BFP8 tile geometry.
// No weight bytes, upload, runtime ownership or device execution are implied.
struct PioneerQ38HybridDenseQ8MatrixPlan {
 using Parent=PioneerQ38HybridHeadModelPlan;using Base=PioneerQ38HybridModelPlan;
 enum : unsigned {DenseQ8Bfp8=10};
 using Proposal=Parent::Proposal;
 template<class Canonical>static bool Build(const Canonical& source,Proposal& output){
  Proposal prior;if(!Parent::Build(source,prior) || prior.parts.size()!=source.Count())return false;
  Proposal staged;staged.parts.reserve(source.Count());unsigned fragments=0;uint32_t rows[16][3]={};
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);const auto& q=prior.parts[i];
   if(!p || q.original!=i || q.bank!=p->bank || q.bank>=7 || !q.bytes || q.bytes>q.allocated || (q.allocated&4095))return false;
   uint64_t bytes=q.bytes,allocated=q.allocated;unsigned encoding=q.encoding;
   if(p->kind==Canonical::WeightKind && p->layer<64 && p->layer%4==3 && p->role>=5 && p->role<=7){
    const unsigned layer=p->layer/4,role=p->role-5,expectedRows=role?1024:12288;
    if(p->type!=8 || !p->rows || ((p->firstRow|p->rows)&31) || p->firstRow!=rows[layer][role]
     || p->rows>expectedRows-p->firstRow || p->rowBytes!=5760 || uint64_t(p->rows)*5760!=p->bytes
     || q.encoding!=Base::Original)return false;
    bytes=uint64_t(p->rows)*5440;allocated=(bytes+4095)&~uint64_t(4095);
    rows[layer][role]+=p->rows;fragments++;encoding=DenseQ8Bfp8;
   }
   auto& at=staged.used[p->bank];
   if(!bytes || bytes>UINT32_MAX || allocated>UINT32_MAX || at>Canonical::Capacity || allocated>Canonical::Capacity-at)return false;
   staged.parts.push_back({i,p->bank,uint32_t(at),uint32_t(bytes),uint32_t(allocated),encoding});at+=allocated;
  }
  for(unsigned layer=0;layer<16;layer++)if(rows[layer][0]!=12288 || rows[layer][1]!=1024 || rows[layer][2]!=1024)return false;
  if(fragments!=16*3*7 || staged.parts.size()!=source.Count())return false;
  output=std::move(staged);return true;
 }
};
#endif
