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
  // The converted stripe is smaller than its old reserved stripe. Keep
  // EVERY original QGW8 address, including FFN, head, KV, recurrent state
  // and workspace, and leave the unused tail as a hole. Compacting these
  // holes would invalidate all of their independently sealed metadata.
  Proposal staged;staged.parts.reserve(source.Count());staged.used=prior.used;unsigned fragments=0;uint32_t rows[16][3]={};
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
   if(!bytes || bytes>UINT32_MAX || allocated>q.allocated || q.offset>Canonical::Capacity
    || q.allocated>Canonical::Capacity-q.offset || uint64_t(q.offset)+q.allocated>prior.used[q.bank])return false;
   staged.parts.push_back({i,p->bank,q.offset,uint32_t(bytes),uint32_t(allocated),encoding});
  }
  for(unsigned layer=0;layer<16;layer++)if(rows[layer][0]!=12288 || rows[layer][1]!=1024 || rows[layer][2]!=1024)return false;
  if(fragments!=16*3*7 || staged.parts.size()!=source.Count())return false;
  output=std::move(staged);return true;
 }
 // Explicitly derive the FFN table from the candidate rather than assuming
 // address preservation; current construction intentionally preserves it.
 // This derives geometry only; the caller must authenticate and upload the
 // exact converted pages before any device owner may use these addresses.
 enum : unsigned {FfnManifestWords=64*3*7};
 template<class Canonical>static bool FfnManifest(const Canonical& source,const Proposal& proposal,uint32_t* output,unsigned words){
  if(!output || words!=FfnManifestWords || (reinterpret_cast<uintptr_t>(output)&3)
   || proposal.parts.size()>SIZE_MAX/sizeof(Base::Part))return false;
  const uintptr_t o=reinterpret_cast<uintptr_t>(output),c=reinterpret_cast<uintptr_t>(&source),p=reinterpret_cast<uintptr_t>(&proposal);
  const uintptr_t storage=reinterpret_cast<uintptr_t>(proposal.parts.data()),limit=~uintptr_t(0);
  const size_t bytes=FfnManifestWords*sizeof(uint32_t),partBytes=proposal.parts.size()*sizeof(Base::Part);
  if(o>limit-bytes || c>limit-sizeof(source) || p>limit-sizeof(proposal) || storage>limit-partBytes
   || (o<c+sizeof(source) && c<o+bytes) || (o<p+sizeof(proposal) && p<o+bytes)
   || (partBytes && o<storage+partBytes && storage<o+bytes))return false;
  Proposal expected;if(!Build(source,expected) || expected.used!=proposal.used || expected.parts.size()!=proposal.parts.size())return false;
  for(unsigned i=0;i<expected.parts.size();i++){
   const auto& a=expected.parts[i];const auto& b=proposal.parts[i];
   if(a.original!=b.original || a.bank!=b.bank || a.offset!=b.offset || a.bytes!=b.bytes || a.allocated!=b.allocated || a.encoding!=b.encoding)return false;
  }
  uint32_t staged[FfnManifestWords]={};unsigned count=0;
  for(unsigned i=0;i<source.Count();i++){
   const auto* part=source.Part(i);if(!part)return false;
   if(part->kind!=Canonical::WeightKind || part->layer>=64 || part->role<2 || part->role>4)continue;
   const auto& dest=expected.parts[i];const unsigned at=(part->layer*3+part->role-2)*7+part->bank;
   if(part->bank>=7 || at>=FfnManifestWords || staged[at] || dest.bank!=part->bank || dest.offset>UINT32_MAX-Canonical::Base)return false;
   staged[at]=Canonical::Base+dest.offset;count++;
  }
  if(count!=FfnManifestWords)return false;
  for(unsigned i=0;i<FfnManifestWords;i++)if(!staged[i])return false;
  for(unsigned i=0;i<FfnManifestWords;i++)output[i]=staged[i];
  return true;
 }
};
#endif
