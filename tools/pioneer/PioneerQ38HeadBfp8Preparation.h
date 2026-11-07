/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HEAD_BFP8_PREPARATION_H
#define PIONEER_Q38_HEAD_BFP8_PREPARATION_H
#include "PioneerQ38HeadBfp8FragmentCache.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <functional>

// Prepare or verify exactly seven original-head stripes. Caller independently
// authenticates the same-open model and converter fingerprint. Disk DATA only;
// never uploads, owns a device, computes logits or enables Work.
template<class Source,class PackedPlan,template<class>class Fragment=PioneerQ38HeadBfp8FragmentCache>
struct PioneerQ38HeadBfp8Preparation {
 using Cache=PioneerModelWeightCache;
 struct Statistics {unsigned fragments=0,hits=0;uint64_t bytes=0;};
 static bool Run(Source& source,const PackedPlan& plan,const PioneerQ38CanonicalModelPlan& canonical,
  const Cache::Digest& digest,const Cache::Digest& fingerprint,const char* directory,bool existingOnly,
  Statistics& output,std::string& error,const std::function<void(unsigned,unsigned,bool)>& progress={}){
  auto fail=[&](const char* why){error=why;return false;};
  auto nonzero=[](const Cache::Digest& h){return std::any_of(h.begin(),h.end(),[](uint8_t v){return v!=0;});};
  if(!directory || directory[0]!='/' || !source.Ready() || !source.Stable() || !source.Epoch()
   || !plan.MatchesSource(source) || !plan.Tiled() || plan.DeltaSlots()!=2 || !canonical.Ready()
   || !nonzero(digest) || !nonzero(fingerprint) || plan.Parts().size()!=canonical.Count())return fail("Invalid head BFP8 preparation source or plan.");
  const uint64_t epoch=source.Epoch();auto original=plan.Parts();if(original.size()!=canonical.Count())return fail("Original plan became unavailable.");
  unsigned found=0,rows=0,banks=0;PioneerQ38Tensor expected;if(!PioneerQ38Expected(64,1,expected))return fail("Output tensor identity unavailable.");
  // Validate every original part and all seven head stripes BEFORE a write.
  for(unsigned i=0;i<canonical.Count();i++){
   const auto* c=canonical.Part(i);const auto& p=original[i];
   if(!c || p.kind!=c->kind || p.layer!=c->layer || p.role!=c->role || p.bank!=c->bank || p.firstRow!=c->firstRow
    || p.rows!=c->rows || p.rowBytes!=c->rowBytes || p.proposedAddress!=canonical.Base+c->offset
    || p.bytes!=c->bytes || p.allocated!=c->allocated)return fail("Original head and canonical layouts differ.");
   if(c->kind!=canonical.WeightKind || c->layer!=64 || c->role!=1)continue;
   if(p.tensor>=source.Tensors().size() || p.bank>=7 || (banks&(1U<<p.bank)) || p.firstRow!=rows
    || !p.rows || (p.rows&63) || p.rowBytes!=4240 || uint64_t(p.rows)*p.rowBytes!=p.bytes)return fail("Head stripe geometry refused.");
   const auto& t=source.Tensors()[p.tensor];
   if(t.name!=expected.name || t.type!=14 || t.rank!=2 || t.inputs!=5120 || t.outputs!=248320 || t.bytes!=expected.bytes)
    return fail("Original output tensor identity refused.");
   rows+=p.rows;banks|=1U<<p.bank;found++;
  }
  if(found!=7 || rows!=248320 || banks!=127)return fail("Head stripe coverage incomplete.");
  Statistics staged;
  for(unsigned i=0;i<canonical.Count();i++){
   const auto& p=original[i];if(p.kind!=canonical.WeightKind || p.layer!=64 || p.role!=1)continue;
   if(!source.Ready() || !source.Stable() || source.Epoch()!=epoch || !plan.MatchesSource(source))return fail("Original source changed during head preparation.");
   const auto& current=plan.Parts();if(current.size()!=original.size())return fail("Original plan became unavailable.");
   for(unsigned k=0;k<original.size();k++){
    const auto& a=original[k];const auto& b=current[k];
    if(a.kind!=b.kind || a.layer!=b.layer || a.role!=b.role || a.tensor!=b.tensor || a.bank!=b.bank
     || a.firstRow!=b.firstRow || a.rows!=b.rows || a.rowBytes!=b.rowBytes || a.proposedAddress!=b.proposedAddress
     || a.bytes!=b.bytes || a.allocated!=b.allocated)return fail("Original plan changed during head preparation.");
   }
   const auto& q=current[i];
   if(q.kind!=p.kind || q.layer!=p.layer || q.role!=p.role || q.tensor!=p.tensor || q.bank!=p.bank
    || q.firstRow!=p.firstRow || q.rows!=p.rows || q.rowBytes!=p.rowBytes || q.proposedAddress!=p.proposedAddress
    || q.bytes!=p.bytes || q.allocated!=p.allocated)return fail("Head stripe changed during preparation.");
   Fragment<Source> cache;bool hit=true;
   if(existingOnly){if(!cache.OpenPrepared(source,p,digest,fingerprint,directory,error))return false;}
   else if(!cache.Prepare(source,p,digest,fingerprint,directory,hit,error))return false;
   if(!cache.Ready() || !source.Stable() || source.Epoch()!=epoch)return fail("Head source or cache changed during preparation.");
   staged.fragments++;staged.hits+=hit;staged.bytes+=uint64_t(p.rows)*5440;
   if(progress)progress(staged.fragments,7,hit);
  }
  if(staged.fragments!=7 || staged.bytes!=UINT64_C(1350860800) || !source.Ready() || !source.Stable()
   || source.Epoch()!=epoch || !plan.MatchesSource(source))return fail("Head preparation stopped before complete coverage.");
  const auto& final=plan.Parts();if(final.size()!=original.size())return fail("Original plan changed after preparation.");
  for(unsigned i=0;i<original.size();i++){
   const auto& p=original[i];const auto& q=final[i];
   if(q.kind!=p.kind || q.layer!=p.layer || q.role!=p.role || q.tensor!=p.tensor || q.bank!=p.bank
    || q.firstRow!=p.firstRow || q.rows!=p.rows || q.rowBytes!=p.rowBytes || q.proposedAddress!=p.proposedAddress
    || q.bytes!=p.bytes || q.allocated!=p.allocated)return fail("Original plan changed after preparation.");
  }
  output=staged;error.clear();return true;
 }
};
#endif
