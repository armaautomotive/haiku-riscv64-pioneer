/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_MODEL_PLAN_H
#define PIONEER_Q38_HYBRID_MODEL_PLAN_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid layout proposal is offline data, not a driver or Work capability
#endif
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

// Repack every part in canonical order within its EXISTING bank. Selected
// FFN stripes use BFP4 or native BFP8 tile geometry; BFP4 retains the
// original allocation length but not the original Q4 encoding. The resulting offsets are NEW, not
// authorized by the old canonical/native owner. This data-only proposal does
// not upload, initialize, authenticate, grant or touch any device resource.
struct PioneerQ38HybridModelPlan {
 enum Encoding : uint32_t {Original=0,Bfp4=4,Bfp8=8};
 struct Part {uint32_t original,bank,offset,bytes,allocated,encoding;};
 struct Proposal {std::vector<Part> parts;std::array<uint64_t,7> used{};uint64_t bfp8Layers=0;};
 template<class Canonical>static bool Build(const Canonical& source,uint64_t bfp8Layers,Proposal& output){
  if(!source.Ready() || !source.Count() || source.Count()>6144)return false;
  Proposal stage;stage.bfp8Layers=bfp8Layers;stage.parts.reserve(source.Count());
  std::array<uint64_t,7> oldUsed{};unsigned converted=0;
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);
   if(!p || p->bank>=7 || !p->bytes || p->bytes>p->allocated || (p->offset&4095) || (p->allocated&4095)
    || p->offset!=oldUsed[p->bank] || p->allocated-p->bytes>=4096 || p->allocated>Canonical::Capacity
    || oldUsed[p->bank]>Canonical::Capacity-p->allocated)return false;
   oldUsed[p->bank]+=p->allocated;
   const bool ffn=p->kind==Canonical::WeightKind && p->type==12 && p->layer<64 && p->role>=2 && p->role<=4;
   const bool select=ffn && (bfp8Layers&(uint64_t(1)<<p->layer));
   uint64_t bytes=p->bytes,allocated=p->allocated;
   if(select){
    const uint64_t width=p->role==4?17408:5120;
    if(!p->rows || ((p->firstRow|p->rows)&31) || p->rowBytes!=width*9/16 || uint64_t(p->rows)*p->rowBytes!=p->bytes)return false;
    bytes=uint64_t(p->rows)*width*17/16;allocated=(bytes+4095)&~uint64_t(4095);converted++;
   }
   auto& at=stage.used[p->bank];
   if(!bytes || bytes>UINT32_MAX || allocated>UINT32_MAX || at>Canonical::Capacity || allocated>Canonical::Capacity-at)return false;
   stage.parts.push_back({i,p->bank,uint32_t(at),uint32_t(bytes),uint32_t(allocated),select?Bfp8:ffn?Bfp4:Original});at+=allocated;
  }
  unsigned selected=0;for(unsigned layer=0;layer<64;layer++)selected+=unsigned((bfp8Layers>>layer)&1);
  if(converted!=selected*3*7)return false;
  for(unsigned b=0;b<7;b++)if(oldUsed[b]!=source.Used(b))return false;
  output=std::move(stage);return true;
 }
};
#endif
