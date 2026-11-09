/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_HEAD_PREPARED_PAGES_H
#define PIONEER_Q38_HYBRID_HEAD_PREPARED_PAGES_H
#if !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC) || PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC) || PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGW8 prepared pages require isolated CPU-only head and FFN preparation roles
#endif
#include "PioneerQ38HybridHeadModelPlan.h"
#include "PioneerQ38Bfp4FragmentCache.h"
#include "PioneerQ38HeadBfp8FragmentCache.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <functional>
#include <memory>

// Read-only page source for the *new* head-BFP8/11-FFN-BFP8 layout.
// The caller authenticates and retains the same original model and independently
// prepares each cache. No device, initialized page, owner, launch or Work grant.
template<class Source,class PackedPlan,class Bfp8,
 template<class>class FourCache=PioneerQ38Bfp4FragmentCache,
 template<class>class HeadCache=PioneerQ38HeadBfp8FragmentCache>
class PioneerQ38HybridHeadPreparedPages {
 using Cache=PioneerModelWeightCache;using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridHeadModelPlan;
 Source* source=nullptr;const PackedPlan* packed=nullptr;Bfp8* bfp8=nullptr;const Canonical* canonical=nullptr;
 Plan::Proposal proposal;std::vector<PioneerQ38PackedPart> parts;std::function<bool()> sourceReady;
 Cache::Digest digest{},fourFingerprint{},headFingerprint{};std::string fourDirectory,headDirectory;
 uint64_t epoch=0;unsigned active=UINT32_MAX;bool attempted=false,failed=false,ready=false,verified=false;
 std::unique_ptr<FourCache<Source>> four;std::unique_ptr<HeadCache<Source>> head;
 static bool Fail(std::string& error,const char* why){error=why;return false;}
 bool Current()const{return ready && !failed && source && packed && bfp8 && canonical && sourceReady && sourceReady()
  && source->Ready() && source->Stable() && source->Epoch()==epoch && packed->MatchesSource(*source)
  && packed->Tiled() && packed->DeltaSlots()==2 && packed->Parts().size()==parts.size()
  && parts.size()==proposal.parts.size() && canonical->Ready() && bfp8->MatchesIdentity(*source,digest);}
 static bool Same(const PioneerQ38PackedPart& a,const PioneerQ38PackedPart& b){
  return a.kind==b.kind && a.layer==b.layer && a.role==b.role && a.tensor==b.tensor && a.bank==b.bank
   && a.firstRow==b.firstRow && a.rows==b.rows && a.rowBytes==b.rowBytes && a.proposedAddress==b.proposedAddress
   && a.bytes==b.bytes && a.allocated==b.allocated;
 }
 static bool Overlaps(const uint8_t* output,size_t bytes,const void* object,size_t length){
  const uintptr_t a=reinterpret_cast<uintptr_t>(output),b=reinterpret_cast<uintptr_t>(object);
  if(!length)return false;
  if(b>UINTPTR_MAX-length)return true;
  return a<b+length && b<a+bytes;
 }
 bool Lose(std::string& error,const char* why){failed=true;four.reset();head.reset();return Fail(error,why);}
public:
 PioneerQ38HybridHeadPreparedPages()=default;
 PioneerQ38HybridHeadPreparedPages(const PioneerQ38HybridHeadPreparedPages&)=delete;
 PioneerQ38HybridHeadPreparedPages& operator=(const PioneerQ38HybridHeadPreparedPages&)=delete;
 bool Bind(Source& input,const PackedPlan& sourcePlan,const Canonical& layout,Bfp8& eight,
  const Cache::Digest& authenticatedSource,const Cache::Digest& ffnFingerprint,const Cache::Digest& outputFingerprint,
  const char* ffnDirectory,const char* outputDirectory,const std::function<bool()>& authenticatedReady,std::string& error){
  if(attempted)return Fail(error,"QGW8 page binding is one-shot.");attempted=true;
  auto nonzero=[](const Cache::Digest& h){return std::any_of(h.begin(),h.end(),[](uint8_t v){return v!=0;});};
  if(!ffnDirectory || ffnDirectory[0]!='/' || !outputDirectory || outputDirectory[0]!='/' || !authenticatedReady || !authenticatedReady()
   || !input.Ready() || !input.Stable() || !input.Epoch() || !sourcePlan.MatchesSource(input) || !sourcePlan.Tiled()
   || sourcePlan.DeltaSlots()!=2 || !layout.Ready() || !eight.MatchesIdentity(input,authenticatedSource)
   || !nonzero(authenticatedSource) || !nonzero(ffnFingerprint) || !nonzero(outputFingerprint))
   return Fail(error,"QGW8 pages require authenticated same-source original and prepared formats.");
  Plan::Proposal staged;if(!Plan::Build(layout,staged) || sourcePlan.Parts().size()!=layout.Count()
   || staged.parts.size()!=layout.Count())return Fail(error,"QGW8 plan geometry does not match.");
  auto originalParts=sourcePlan.Parts();if(originalParts.size()!=layout.Count())return Fail(error,"Original plan became unavailable.");
  for(unsigned i=0;i<layout.Count();i++){
   const auto* c=layout.Part(i);const auto& p=originalParts[i];const auto& dest=staged.parts[i];
   if(!c || p.kind!=c->kind || p.layer!=c->layer || p.role!=c->role || p.bank!=c->bank
    || p.firstRow!=c->firstRow || p.rows!=c->rows || p.rowBytes!=c->rowBytes
    || p.proposedAddress!=Canonical::Base+c->offset || p.bytes!=c->bytes || p.allocated!=c->allocated
    || dest.original!=i || dest.bank!=c->bank)return Fail(error,"Original and QGW8 part identities differ.");
   if(c->kind==Canonical::WeightKind){
    if(p.tensor>=input.Tensors().size())return Fail(error,"Original tensor index is invalid.");
    PioneerQ38Tensor expected;const auto& t=input.Tensors()[p.tensor];
    if(!PioneerQ38Expected(p.layer,p.role,expected) || t.name!=expected.name || t.type!=expected.type
     || t.rank!=expected.rank || t.inputs!=expected.inputs || t.outputs!=expected.outputs || t.bytes!=expected.bytes)
     return Fail(error,"Original tensor identity differs from the fixed model.");
   }
  }
  const auto& finalParts=sourcePlan.Parts();if(finalParts.size()!=originalParts.size())return Fail(error,"Original plan changed while binding.");
  for(unsigned i=0;i<originalParts.size();i++)if(!Same(originalParts[i],finalParts[i]))return Fail(error,"Original plan changed while binding.");
  source=&input;packed=&sourcePlan;canonical=&layout;bfp8=&eight;proposal=std::move(staged);parts=std::move(originalParts);
  sourceReady=authenticatedReady;digest=authenticatedSource;fourFingerprint=ffnFingerprint;headFingerprint=outputFingerprint;
  fourDirectory=ffnDirectory;headDirectory=outputDirectory;epoch=input.Epoch();ready=true;
  if(!Current())return Lose(error,"Source changed while binding QGW8 pages.");error.clear();return true;
 }
 bool Ready()const{return Current();}
 // Identity bridge for a separately validated derived layout. It exposes no
 // pointers or GPU capability and does not authenticate the caller's data.
 bool MatchesBinding(const Source& s,const PackedPlan& p,const Canonical& c,const Bfp8& b,const Cache::Digest& sha)const{
  return Current() && source==&s && packed==&p && canonical==&c && bfp8==&b && digest==sha;
 }
 bool AllVerified()const{
  if(!verified || !Current())return false;const auto& current=packed->Parts();if(current.size()!=parts.size())return false;
  for(unsigned i=0;i<parts.size();i++)if(!Same(current[i],parts[i]))return false;return Current();
 }
 // Out-of-band metadata must never overwrite either the bound page source
 // or the original plan/tensor descriptors. This is deliberately not on
 // the hot per-page path; metadata publication calls it before copying.
 bool OutputSafe(const void* destination,size_t bytes)const{
  const auto* output=static_cast<const uint8_t*>(destination);
  if(!output || !bytes || reinterpret_cast<uintptr_t>(output)>UINTPTR_MAX-bytes || !Current())return false;
  const auto& live=packed->Parts();const auto& tensors=source->Tensors();
  if(live.size()!=parts.size() || live.size()>SIZE_MAX/sizeof(live[0]) || tensors.size()>SIZE_MAX/sizeof(tensors[0])
   || parts.size()>SIZE_MAX/sizeof(parts[0]) || proposal.parts.size()>SIZE_MAX/sizeof(proposal.parts[0]))return false;
  if(Overlaps(output,bytes,this,sizeof(*this)) || Overlaps(output,bytes,source,sizeof(*source))
   || Overlaps(output,bytes,packed,sizeof(*packed)) || Overlaps(output,bytes,bfp8,sizeof(*bfp8))
   || Overlaps(output,bytes,canonical,sizeof(*canonical)) || Overlaps(output,bytes,parts.data(),parts.size()*sizeof(parts[0]))
   || Overlaps(output,bytes,proposal.parts.data(),proposal.parts.size()*sizeof(proposal.parts[0]))
   || Overlaps(output,bytes,live.data(),live.size()*sizeof(live[0]))
   || Overlaps(output,bytes,tensors.data(),tensors.size()*sizeof(tensors[0])))return false;
  for(const auto& tensor:tensors){
   if(tensor.name.capacity()==SIZE_MAX || Overlaps(output,bytes,tensor.name.data(),tensor.name.capacity()+1))return false;
  }
  return Current();
 }
 const PioneerQ38HybridModelPlan::Part* Part(unsigned i)const{return Current() && i<proposal.parts.size()?&proposal.parts[i]:nullptr;}
 bool ReadPages(unsigned fragment,unsigned page,unsigned count,uint8_t* output,unsigned capacity,std::string& error){
  if(!output || !count || count>Cache::ChunkBytes/4096 || capacity<count*4096
   || reinterpret_cast<uintptr_t>(output)>UINTPTR_MAX-count*4096 || fragment>=proposal.parts.size())
   return Fail(error,"Invalid QGW8 page destination or part.");
  if(!Current())return Lose(error,"Authenticated QGW8 source changed.");
  const size_t bytes=size_t(count)*4096;
  if(Overlaps(output,bytes,this,sizeof(*this)) || Overlaps(output,bytes,source,sizeof(*source))
   || Overlaps(output,bytes,packed,sizeof(*packed)) || Overlaps(output,bytes,bfp8,sizeof(*bfp8))
   || Overlaps(output,bytes,canonical,sizeof(*canonical))
   || Overlaps(output,bytes,parts.data(),parts.size()*sizeof(parts[0]))
   || Overlaps(output,bytes,proposal.parts.data(),proposal.parts.size()*sizeof(proposal.parts[0])))
   return Fail(error,"QGW8 page destination overlaps source or routing state.");
  const auto& liveParts=packed->Parts();if(liveParts.size()!=parts.size())return Lose(error,"Original plan became unavailable.");
  const auto& tensors=source->Tensors();
  if(liveParts.size()>SIZE_MAX/sizeof(liveParts[0]) || tensors.size()>SIZE_MAX/sizeof(tensors[0])
   || Overlaps(output,bytes,liveParts.data(),liveParts.size()*sizeof(liveParts[0]))
   || Overlaps(output,bytes,tensors.data(),tensors.size()*sizeof(tensors[0])))
   return Fail(error,"QGW8 page destination overlaps live source descriptors.");
  const auto part=liveParts[fragment];if(!Same(part,parts[fragment]))return Lose(error,"Original plan changed after binding.");
  const auto dest=proposal.parts[fragment];
  if(part.kind!=Canonical::WeightKind || page>=dest.allocated/4096 || count>dest.allocated/4096-page)
   return Fail(error,"QGW8 page range is not valid learned-weight data.");
  std::vector<uint8_t> stage(count*4096,0);
  if(part.layer==64 && part.role==1){
   if(dest.encoding!=Plan::HeadBfp8)return Lose(error,"Head encoding differs from QGW8 plan.");
   if(active!=fragment){auto next=std::make_unique<HeadCache<Source>>();
    if(!next->OpenPrepared(*source,part,digest,headFingerprint,headDirectory.c_str(),error))return Lose(error,"Prepared head BFP8 cache unavailable.");
    head=std::move(next);four.reset();active=fragment;
   }
   if(!head || !head->ReadPages(page,count,stage.data(),unsigned(stage.size()),error))return Lose(error,"Prepared head BFP8 page failed verification.");
  }else if(part.layer<64 && part.role>=2 && part.role<=4){
   if(part.layer>=50 && part.layer<=60){
    if(dest.encoding!=PioneerQ38HybridModelPlan::Bfp8 || !bfp8->ReadStripePages(part.tensor,part.firstRow,part.rows,page,count,stage.data(),unsigned(stage.size()),error))
     return Lose(error,"Prepared FFN BFP8 page unavailable or mismatched.");
   }else{
    if(dest.encoding!=PioneerQ38HybridModelPlan::Bfp4)return Lose(error,"FFN BFP4 encoding differs from QGW8 plan.");
    if(active!=fragment){auto next=std::make_unique<FourCache<Source>>();
     if(!next->OpenPrepared(*source,part,digest,fourFingerprint,fourDirectory.c_str(),error))return Lose(error,"Prepared FFN BFP4 cache unavailable.");
     four=std::move(next);head.reset();active=fragment;
    }
    if(!four || !four->ReadPages(page,count,stage.data(),unsigned(stage.size()),error))return Lose(error,"Prepared FFN BFP4 page failed verification.");
   }
  }else{
   if(dest.encoding!=PioneerQ38HybridModelPlan::Original)return Lose(error,"Original encoding differs from QGW8 plan.");
   for(unsigned i=0;i<count;i++)if(!packed->ReadWeightPage(*source,fragment,page+i,stage.data()+i*4096,4096))return Lose(error,"Original model page unavailable.");
  }
  if(!Current())return Lose(error,"Source changed during QGW8 page read.");
  const auto& finalParts=packed->Parts();if(finalParts.size()!=parts.size() || !Same(finalParts[fragment],part))return Lose(error,"Original plan changed during QGW8 page read.");
  std::memcpy(output,stage.data(),stage.size());error.clear();return true;
 }
 bool ReadPage(unsigned fragment,unsigned page,uint8_t* output,unsigned capacity,std::string& error){return ReadPages(fragment,page,1,output,capacity,error);}
 bool VerifyAll(std::string& error,const std::function<void(unsigned,unsigned)>& progress={}){
  if(!Current() || verified)return Fail(error,"QGW8 preflight needs a fresh live source.");
  unsigned count=0;std::array<uint8_t,4096> page={};
  for(unsigned i=0;i<parts.size();i++){
   const auto& p=parts[i];if(p.kind!=Canonical::WeightKind || !((p.layer<64 && p.role>=2 && p.role<=4) || (p.layer==64 && p.role==1)))continue;
   if(!ReadPage(i,0,page.data(),page.size(),error))return false;
   count++;if(progress)progress(count,1351);
  }
  if(count!=1351 || !Current())return Lose(error,"QGW8 preflight did not cover every converted fragment.");
  verified=true;if(!AllVerified())return Lose(error,"Original model plan changed during QGW8 preflight.");
  error.clear();return true;
 }
};
#endif
