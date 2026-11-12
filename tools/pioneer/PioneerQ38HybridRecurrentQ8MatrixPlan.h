/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent Q8 matrix plan is isolated offline geometry only
#endif
#ifndef PIONEER_Q38_HYBRID_RECURRENT_Q8_MATRIX_PLAN_H
#define PIONEER_Q38_HYBRID_RECURRENT_Q8_MATRIX_PLAN_H
#include "PioneerQ38HybridDenseQ8MatrixPlan.h"

// Geometry for the three large recurrent Q8 projections in each of the 48
// recurrent layers. The two 48-row projections intentionally remain on the
// original path: their BFP8 partial-tile policy is not yet implemented.
// This does not convert bytes, grant upload authority, or run a matrix engine.
struct PioneerQ38HybridRecurrentQ8MatrixPlan {
 using Parent=PioneerQ38HybridDenseQ8MatrixPlan;
 using Proposal=Parent::Proposal;
 enum : unsigned {RecurrentQ8Bfp8=11};
 template<class Canonical>static bool Build(const Canonical& source,Proposal& output){
  Proposal prior;if(!Parent::Build(source,prior) || prior.parts.size()!=source.Count())return false;
  Proposal staged;staged.parts.reserve(source.Count());staged.used=prior.used;
  unsigned fragments=0;uint32_t rows[64][3]={};
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);const auto& q=prior.parts[i];
   if(!p || q.original!=i || q.bank!=p->bank || q.bank>=7 || !q.bytes || q.bytes>q.allocated || (q.allocated&4095))return false;
   uint64_t bytes=q.bytes,allocated=q.allocated;unsigned encoding=q.encoding;
   if(p->kind==Canonical::WeightKind && p->layer<64 && p->layer%4!=3
    && (p->role==5 || p->role==6 || p->role==13)){
    const unsigned slot=p->role==5?0:p->role==6?1:2;
    const unsigned expectedRows=slot==0?6144:slot==1?10240:5120;
    const unsigned oldRowBytes=slot==2?6912:5760,newRowBytes=slot==2?6528:5440;
    if(p->type!=8 || !p->rows || ((p->firstRow|p->rows)&31) || p->firstRow!=rows[p->layer][slot]
     || p->firstRow>expectedRows || p->rows>expectedRows-p->firstRow
     || p->rowBytes!=oldRowBytes || uint64_t(p->rows)*oldRowBytes!=p->bytes
     || q.encoding!=PioneerQ38HybridModelPlan::Original)return false;
    bytes=uint64_t(p->rows)*newRowBytes;allocated=(bytes+4095)&~uint64_t(4095);
    rows[p->layer][slot]+=p->rows;fragments++;encoding=RecurrentQ8Bfp8;
   }
   if(!bytes || bytes>UINT32_MAX || allocated>q.allocated || q.offset>Canonical::Capacity
    || q.allocated>Canonical::Capacity-q.offset || uint64_t(q.offset)+q.allocated>prior.used[q.bank])return false;
   staged.parts.push_back({i,p->bank,q.offset,uint32_t(bytes),uint32_t(allocated),encoding});
  }
  for(unsigned layer=0;layer<64;layer++)if(layer%4!=3
   && (rows[layer][0]!=6144 || rows[layer][1]!=10240 || rows[layer][2]!=5120))return false;
  if(fragments!=48*3*7 || staged.parts.size()!=source.Count())return false;
  output=std::move(staged);return true;
 }
};
#endif
