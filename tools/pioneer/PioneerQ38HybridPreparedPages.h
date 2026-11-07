/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_PREPARED_PAGES_H
#define PIONEER_Q38_HYBRID_PREPARED_PAGES_H
#if !defined(PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC) || PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid prepared pages are experimental userspace data, not Work or device authority
#endif
#include "PioneerQ38Bfp4FragmentCache.h"
#include "PioneerQ38HybridModelPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <functional>
#include <memory>

// Read-only prepared-page adapter for a complete proposed hybrid layout.
// Source/plan and bfp8 must remain alive. Caller independently authenticates
// the SAME source and supplies its digest and a live binding predicate.
// No model arithmetic, accelerator IO, bank ownership or Work selection.
template<class Source,class PackedPlan,class Bfp8,template<class>class FourCache=PioneerQ38Bfp4FragmentCache> class PioneerQ38HybridPreparedPages {
 using Cache=PioneerModelWeightCache;using Canonical=PioneerQ38CanonicalModelPlan;
 Source* source=nullptr;const PackedPlan* packed=nullptr;Bfp8* bfp8=nullptr;
 const Canonical* canonical=nullptr;PioneerQ38HybridModelPlan::Proposal proposal;
 std::vector<PioneerQ38PackedPart> sourceParts;
 std::function<bool()> sourceReady;Cache::Digest digest{},implementation{};
 std::string directory;uint64_t epoch=0;bool attempted=false,failed=false,ready=false;
 unsigned active=UINT32_MAX;std::unique_ptr<FourCache<Source>> four;bool verified=false;
 static bool Fail(std::string& error,const char* reason){error=reason;return false;}
 bool Current()const{return ready && !failed && source && packed && bfp8 && canonical && sourceReady && sourceReady()
  && source->Ready() && source->Stable() && source->Epoch()==epoch && packed->MatchesSource(*source)
  && packed->Tiled() && packed->DeltaSlots()==2 && packed->Parts().size()==sourceParts.size()
  && sourceParts.size()==proposal.parts.size() && canonical->Ready() && bfp8->MatchesIdentity(*source,digest);}
 static bool Same(const PioneerQ38PackedPart& a,const PioneerQ38PackedPart& b){
  return a.kind==b.kind && a.layer==b.layer && a.role==b.role && a.tensor==b.tensor && a.bank==b.bank
   && a.firstRow==b.firstRow && a.rows==b.rows && a.rowBytes==b.rowBytes && a.proposedAddress==b.proposedAddress
   && a.bytes==b.bytes && a.allocated==b.allocated;
 }
 bool Lose(std::string& error,const char* reason){failed=true;four.reset();return Fail(error,reason);}
public:
 enum {MaskHigh=0x1fff0000};
 PioneerQ38HybridPreparedPages()=default;
 PioneerQ38HybridPreparedPages(const PioneerQ38HybridPreparedPages&)=delete;
 PioneerQ38HybridPreparedPages& operator=(const PioneerQ38HybridPreparedPages&)=delete;
 bool Bind(Source& input,const PackedPlan& sourcePlan,const Canonical& layout,Bfp8& eight,
  const Cache::Digest& authenticatedSource,const Cache::Digest& fingerprint,const char* cacheDirectory,
  const std::function<bool()>& authenticatedReady,std::string& error){
  if(attempted)return Fail(error,"Hybrid page source binding is one-shot.");
  attempted=true;
  if(!cacheDirectory || cacheDirectory[0]!='/' || !authenticatedReady || !authenticatedReady()
   || !input.Ready() || !input.Stable() || !input.Epoch() || !sourcePlan.MatchesSource(input)
   || !sourcePlan.Tiled() || sourcePlan.DeltaSlots()!=2 || !layout.Ready() || !eight.MatchesIdentity(input,authenticatedSource)
   || !std::any_of(authenticatedSource.begin(),authenticatedSource.end(),[](uint8_t v){return v!=0;})
   || !std::any_of(fingerprint.begin(),fingerprint.end(),[](uint8_t v){return v!=0;}))
   return Fail(error,"Hybrid pages require the same authenticated source, tiled 8192/two-slot plan and prepared BFP8 source.");
  PioneerQ38HybridModelPlan::Proposal staged;
  if(!PioneerQ38HybridModelPlan::Build(layout,UINT64_C(0x1fff000000000000),staged)
   || sourcePlan.Parts().size()!=layout.Count() || staged.parts.size()!=layout.Count())
   return Fail(error,"Hybrid plan and original model geometry do not match.");
  for(unsigned i=0;i<layout.Count();i++){
   const auto* c=layout.Part(i);const auto& p=sourcePlan.Parts()[i];const auto& dest=staged.parts[i];
   if(!c || p.kind!=c->kind || p.layer!=c->layer || p.role!=c->role || p.bank!=c->bank
    || p.firstRow!=c->firstRow || p.rows!=c->rows || p.rowBytes!=c->rowBytes
    || p.proposedAddress!=Canonical::Base+c->offset || p.bytes!=c->bytes || p.allocated!=c->allocated
    || dest.original!=i || dest.bank!=c->bank)return Fail(error,"Original and proposed part identities differ.");
   if(c->kind==Canonical::WeightKind){
    if(p.tensor>=input.Tensors().size())return Fail(error,"Original tensor index is invalid.");
    PioneerQ38Tensor expected;const auto& t=input.Tensors()[p.tensor];
    if(!PioneerQ38Expected(p.layer,p.role,expected) || t.name!=expected.name || t.type!=expected.type
     || t.rank!=expected.rank || t.inputs!=expected.inputs || t.outputs!=expected.outputs || t.bytes!=expected.bytes)
     return Fail(error,"Original tensor identity differs from the fixed model.");
   }
  }
  source=&input;packed=&sourcePlan;canonical=&layout;bfp8=&eight;proposal=std::move(staged);sourceParts=sourcePlan.Parts();
  sourceReady=authenticatedReady;digest=authenticatedSource;implementation=fingerprint;directory=cacheDirectory;epoch=input.Epoch();ready=true;
  if(!Current())return Lose(error,"Source changed while binding hybrid pages.");
  error.clear();return true;
 }
 bool Ready()const{return Current();}
 bool AllVerified()const{
  if(!verified || !Current())return false;
  // Parts() checks the model's file identity on every call. Capture the
  // already-owned plan once, compare every fragment, then recheck the source
  // and plan binding. Calling Parts() per element causes thousands of file
  // identity checks on every upload checkpoint and can index an empty view if
  // that identity is lost in the middle of the loop.
  const auto& parts=packed->Parts();
  if(parts.size()!=sourceParts.size())return false;
  for(unsigned i=0;i<sourceParts.size();i++)if(!Same(parts[i],sourceParts[i]))return false;
  return Current(); // preflight only; later cache reads still check integrity
 }
 const PioneerQ38HybridModelPlan::Part* Part(unsigned i)const{return Current() && i<proposal.parts.size()?&proposal.parts[i]:nullptr;}
 bool ReadPages(unsigned fragment,unsigned page,unsigned count,uint8_t* output,unsigned capacity,std::string& error){
  if(!output || !count || count>Cache::ChunkBytes/4096 || capacity<count*4096
   || reinterpret_cast<uintptr_t>(output)>UINTPTR_MAX-count*4096
   || fragment>=proposal.parts.size())return Fail(error,"Invalid hybrid page destination or part.");
  if(!Current())return Lose(error,"Authenticated model or prepared source changed.");
  const auto& dest=proposal.parts[fragment];const auto part=packed->Parts()[fragment];
  if(!Same(part,sourceParts[fragment]))return Lose(error,"Original model plan changed after binding.");
  if(part.kind!=Canonical::WeightKind || page>=dest.allocated/4096 || count>dest.allocated/4096-page)
   return Fail(error,"Hybrid page range is not valid learned-weight data.");
  std::vector<uint8_t> stage(count*4096,0);
  if(part.layer<64 && part.role>=2 && part.role<=4){
   if(part.layer>=48 && part.layer<=60){
    if(dest.encoding!=PioneerQ38HybridModelPlan::Bfp8)return Lose(error,"BFP8 fragment encoding differs from the plan.");
    if(!bfp8->ReadStripePages(part.tensor,part.firstRow,part.rows,page,count,stage.data(),unsigned(stage.size()),error))
     return Lose(error,"Prepared BFP8 stripe is missing, changed or unreadable.");
   }else{
    if(dest.encoding!=PioneerQ38HybridModelPlan::Bfp4)return Lose(error,"BFP4 fragment encoding differs from the plan.");
    if(active!=fragment){
     auto next=std::make_unique<FourCache<Source>>();
     if(!next->OpenPrepared(*source,part,digest,implementation,directory.c_str(),error))
      return Lose(error,"Prepared BFP4 fragment is missing, changed or unreadable.");
     four=std::move(next);active=fragment;
    }
    if(!four || !four->ReadPages(page,count,stage.data(),unsigned(stage.size()),error))
     return Lose(error,"Prepared BFP4 page range failed verification.");
   }
  }else{
   if(dest.encoding!=PioneerQ38HybridModelPlan::Original)return Lose(error,"Original-format fragment encoding differs from the plan.");
   for(unsigned i=0;i<count;i++)if(!packed->ReadWeightPage(*source,fragment,page+i,stage.data()+i*4096,4096))
    return Lose(error,"Original-format model page could not be read.");
  }
  if(!Current() || !Same(packed->Parts()[fragment],part))return Lose(error,"Authenticated model or plan changed during hybrid page read.");
  std::memcpy(output,stage.data(),stage.size());error.clear();return true;
 }
 bool ReadPage(unsigned fragment,unsigned page,uint8_t* output,unsigned capacity,std::string& error){
  return ReadPages(fragment,page,1,output,capacity,error);
 }
 // Expensive read-only preflight BEFORE any future device acquisition: open
 // and verify every converted fragment/tensor. Original pages are covered by
 // the caller's full-file authentication. Later reads still recheck integrity.
 bool VerifyAll(std::string& error,const std::function<void(unsigned,unsigned)>& progress={}){
  if(!Current() || verified)return Fail(error,"Hybrid preflight needs a fresh live bound source.");
  unsigned count=0;std::array<uint8_t,4096> page={};
  for(unsigned i=0;i<sourceParts.size();i++){
   const auto& p=sourceParts[i];if(p.kind!=Canonical::WeightKind || p.layer>=64 || p.role<2 || p.role>4)continue;
   if(!ReadPage(i,0,page.data(),page.size(),error))return false;
   count++;if(progress)progress(count,64*3*7);
  }
  if(count!=64*3*7 || !Current())return Lose(error,"Hybrid preflight did not verify every converted fragment.");
  verified=true;if(!AllVerified())return Lose(error,"Original model plan changed during hybrid preflight.");
  error.clear();return true;
 }
};
#endif
