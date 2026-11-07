/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_BFP4_PREPARATION_H
#define PIONEER_Q38_HYBRID_BFP4_PREPARATION_H
#include "PioneerQ38Bfp4FragmentCache.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <functional>

// Prepare or verify all 1071 BFP4 fragments for the fixed hybrid layer mask.
// The caller supplies an independently authenticated, stable same-open model;
// no accelerator, mutable-state upload, inference or quality is implied.
template<class Source,class Plan,template<class>class Fragment=PioneerQ38Bfp4FragmentCache>
struct PioneerQ38HybridBfp4Preparation {
 struct Statistics {unsigned fragments=0,hits=0;uint64_t bytes=0;};
 static bool Run(Source& source,const Plan& plan,const PioneerQ38CanonicalModelPlan& canonical,
  const PioneerModelWeightCache::Digest& digest,const PioneerModelWeightCache::Digest& fingerprint,
  const char* directory,bool existingOnly,Statistics& result,std::string& error,
  const std::function<void(unsigned,unsigned,bool)>& progress={}){
  auto fail=[&](const char* why){error=why;return false;};
  if(!directory || directory[0]!='/' || !source.Ready() || !source.Stable() || !source.Epoch()
   || !plan.MatchesSource(source) || !plan.Tiled() || plan.DeltaSlots()!=2 || !canonical.Ready()
   || plan.Parts().size()!=canonical.Count())return fail("Invalid hybrid preparation source or plan.");
  const uint64_t epoch=source.Epoch();unsigned expected=0;
  // Validate EVERY selected original stripe before the first cache write.
  for(unsigned i=0;i<canonical.Count();i++){
   const auto* c=canonical.Part(i);const auto& p=plan.Parts()[i];
   if(!c || p.kind!=c->kind || p.layer!=c->layer || p.role!=c->role || p.bank!=c->bank || p.firstRow!=c->firstRow
    || p.rows!=c->rows || p.rowBytes!=c->rowBytes || p.proposedAddress!=canonical.Base+c->offset
    || p.bytes!=c->bytes || p.allocated!=c->allocated)return fail("Original and canonical model layouts differ.");
   if(c->kind==canonical.WeightKind && c->layer<64 && c->role>=2 && c->role<=4 && (c->layer<48 || c->layer>60)){
    PioneerQ38Tensor expectedTensor;
    if(p.tensor>=source.Tensors().size() || !PioneerQ38Expected(p.layer,p.role,expectedTensor))return fail("Selected tensor is invalid.");
    const auto& tensor=source.Tensors()[p.tensor];
    if(tensor.name!=expectedTensor.name || tensor.type!=expectedTensor.type || tensor.rank!=expectedTensor.rank
     || tensor.inputs!=expectedTensor.inputs || tensor.outputs!=expectedTensor.outputs || tensor.bytes!=expectedTensor.bytes)
     return fail("Selected tensor identity differs from the fixed model.");
    expected++;
   }
  }
  if(expected!=1071)return fail("Hybrid BFP4 fragment count is not canonical.");
  Statistics staged;
  for(unsigned i=0;i<canonical.Count();i++){
   if(plan.Parts().size()!=canonical.Count())return fail("Original plan changed during preparation.");
   const auto p=plan.Parts()[i];const auto* c=canonical.Part(i);
   if(!c || p.kind!=c->kind || p.layer!=c->layer || p.role!=c->role || p.bank!=c->bank || p.firstRow!=c->firstRow
    || p.rows!=c->rows || p.rowBytes!=c->rowBytes || p.proposedAddress!=canonical.Base+c->offset
    || p.bytes!=c->bytes || p.allocated!=c->allocated)return fail("Original plan changed during preparation.");
   if(p.kind!=canonical.WeightKind || p.layer>=64 || p.role<2 || p.role>4 || (p.layer>=48 && p.layer<=60))continue;
   if(!source.Ready() || !source.Stable() || source.Epoch()!=epoch || !plan.MatchesSource(source))return fail("Model changed during hybrid preparation.");
   Fragment<Source> fragment;bool hit=true;
   if(existingOnly){if(!fragment.OpenPrepared(source,p,digest,fingerprint,directory,error))return false;}
   else if(!fragment.Prepare(source,p,digest,fingerprint,directory,hit,error))return false;
   if(!fragment.Ready() || !source.Stable() || source.Epoch()!=epoch)return fail("Model or converted cache changed during preparation.");
   staged.fragments++;staged.hits+=hit;staged.bytes+=p.allocated;
   if(progress)progress(staged.fragments,expected,hit);
  }
  if(staged.fragments!=expected || !source.Ready() || !source.Stable() || source.Epoch()!=epoch || !plan.MatchesSource(source))
   return fail("Hybrid preparation stopped before complete verified coverage.");
  result=staged;error.clear();return true;
 }
};
#endif
