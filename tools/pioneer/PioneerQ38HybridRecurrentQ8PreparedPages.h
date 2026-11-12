/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent Q8 prepared pages are isolated offline data only
#endif
#ifndef PIONEER_Q38_HYBRID_RECURRENT_Q8_PREPARED_PAGES_H
#define PIONEER_Q38_HYBRID_RECURRENT_Q8_PREPARED_PAGES_H
#include "PioneerQ38HybridRecurrentQ8MatrixPlan.h"
#include "PioneerQ38HybridDenseQ8PreparedPages.h"

// Routes already-verified recurrent BFP8 cache pages through an existing
// verified dense-Q8 parent. It does not create caches, upload, execute or
// establish device authority. All original source identities remain bound.
template<class Source,class PackedPlan,class Bfp8,class ParentPages>
class PioneerQ38HybridRecurrentQ8PreparedPages {
 using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridRecurrentQ8MatrixPlan;
 using Base=PioneerQ38HybridModelPlan;using Cache=PioneerModelWeightCache;
 Source* source=nullptr;const PackedPlan* packed=nullptr;const Canonical* canonical=nullptr;
 Bfp8* eight=nullptr;ParentPages* parent=nullptr;Cache::Digest digest{};
 Plan::Proposal proposal;std::vector<PioneerQ38PackedPart> parts;
 bool attempted=false,ready=false,verified=false,failed=false;
 static bool Fail(std::string& error,const char* why){error=why;return false;}
 bool Current()const{return ready && !failed && source && packed && canonical && eight && parent
  && parent->MatchesBinding(*source,*packed,*canonical,digest) && eight->MatchesIdentity(*source,digest)
  && parts.size()==proposal.parts.size() && packed->Parts().size()==parts.size();}
 bool Lose(std::string& error,const char* why){failed=true;return Fail(error,why);}
 static bool Same(const PioneerQ38PackedPart& a,const PioneerQ38PackedPart& b){
  return a.kind==b.kind && a.layer==b.layer && a.role==b.role && a.tensor==b.tensor && a.bank==b.bank
   && a.firstRow==b.firstRow && a.rows==b.rows && a.rowBytes==b.rowBytes && a.proposedAddress==b.proposedAddress
   && a.bytes==b.bytes && a.allocated==b.allocated;
 }
 static bool Overlap(const void* a,size_t n,const void* b,size_t m){
  const uintptr_t x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);
  return (n && x>UINTPTR_MAX-n) || (m && y>UINTPTR_MAX-m) || (n && m && x<y+m && y<x+n);
 }
public:
 PioneerQ38HybridRecurrentQ8PreparedPages()=default;
 PioneerQ38HybridRecurrentQ8PreparedPages(const PioneerQ38HybridRecurrentQ8PreparedPages&)=delete;
 PioneerQ38HybridRecurrentQ8PreparedPages& operator=(const PioneerQ38HybridRecurrentQ8PreparedPages&)=delete;
 bool Bind(Source& s,const PackedPlan& p,const Canonical& c,Bfp8& b,ParentPages& base,
  const Cache::Digest& sha,std::string& error){
  if(attempted)return Fail(error,"Recurrent Q8 page binding is one-shot.");attempted=true;
  if(!base.MatchesBinding(s,p,c,sha) || !base.AllVerified() || !b.MatchesIdentity(s,sha))
   return Fail(error,"Recurrent Q8 pages require the same verified parent and BFP8 source.");
  Plan::Proposal staged,expectedParent;
  if(!Plan::Build(c,staged) || !Plan::Parent::Build(c,expectedParent) || staged.parts.size()!=p.Parts().size()
   || expectedParent.parts.size()!=staged.parts.size())return Fail(error,"Recurrent Q8 geometry differs from source.");
  auto original=p.Parts();
  for(unsigned i=0;i<original.size();i++){
   const auto* old=base.Part(i);const auto* logical=c.Part(i);const auto& part=original[i];const auto& expected=expectedParent.parts[i];
   if(!old || !logical || old->original!=expected.original || old->bank!=expected.bank || old->offset!=expected.offset
    || old->bytes!=expected.bytes || old->allocated!=expected.allocated || old->encoding!=expected.encoding
    || part.kind!=logical->kind || part.layer!=logical->layer || part.role!=logical->role || part.bank!=logical->bank
    || part.firstRow!=logical->firstRow || part.rows!=logical->rows || part.rowBytes!=logical->rowBytes
    || part.bytes!=logical->bytes || part.allocated!=logical->allocated
    || part.proposedAddress!=Canonical::Base+logical->offset || staged.parts[i].original!=i)
    return Fail(error,"Recurrent Q8 source or parent snapshot differs.");
  }
  if(!base.MatchesVerifiedSnapshot(original))return Fail(error,"Recurrent Q8 and parent snapshots differ.");
  source=&s;packed=&p;canonical=&c;eight=&b;parent=&base;digest=sha;proposal=std::move(staged);parts=std::move(original);ready=true;
  if(!Current())return Lose(error,"Recurrent Q8 source changed during binding.");error.clear();return true;
 }
 bool Ready()const{return Current();}
 bool AllVerified()const{return verified && Current() && parent->AllVerified() && Current();}
 const Base::Part* Part(unsigned i)const{return Current() && i<proposal.parts.size()?&proposal.parts[i]:nullptr;}
 bool OutputSafe(const void* out,size_t bytes)const{
  return out && bytes && Current() && parent->OutputSafe(out,bytes)
   && !Overlap(out,bytes,eight,sizeof(*eight)) && !Overlap(out,bytes,this,sizeof(*this))
   && !Overlap(out,bytes,parts.data(),parts.size()*sizeof(parts[0]))
   && !Overlap(out,bytes,proposal.parts.data(),proposal.parts.size()*sizeof(proposal.parts[0]));
 }
 bool ReadPages(unsigned fragment,unsigned page,unsigned count,uint8_t* out,unsigned capacity,std::string& error){
  if(!count || count>16 || !out || capacity<count*4096 || fragment>=proposal.parts.size())
   return Fail(error,"Invalid recurrent Q8 destination or fragment.");
  if(!Current())return Lose(error,"Recurrent Q8 source changed.");
  if(!OutputSafe(out,size_t(count)*4096))return Fail(error,"Recurrent Q8 destination overlaps source state.");
  const auto& live=packed->Parts();const auto part=live[fragment];const auto dest=proposal.parts[fragment];
  if(!Same(part,parts[fragment]))return Lose(error,"Recurrent Q8 original part changed.");
  if(part.kind!=Canonical::WeightKind || page>=dest.allocated/4096 || count>dest.allocated/4096-page)
   return Fail(error,"Recurrent Q8 page range is invalid.");
  std::vector<uint8_t> stage(count*4096,0);
  if(dest.encoding==Plan::RecurrentQ8Bfp8){
   if(!eight->ReadStripePages(part.tensor,part.firstRow,part.rows,page,count,stage.data(),unsigned(stage.size()),error))
    return Lose(error,"Prepared recurrent Q8 BFP8 stripe unavailable.");
  }else if(!parent->ReadPages(fragment,page,count,stage.data(),unsigned(stage.size()),error))
   return Lose(error,"Parent dense Q8 page unavailable.");
  if(!Current() || !Same(packed->Parts()[fragment],part))return Lose(error,"Recurrent Q8 source changed during read.");
  std::memcpy(out,stage.data(),stage.size());error.clear();return true;
 }
 bool ReadPage(unsigned fragment,unsigned page,uint8_t* out,unsigned capacity,std::string& error){return ReadPages(fragment,page,1,out,capacity,error);}
 bool VerifyAll(std::string& error){
  if(!Current() || verified || !parent->AllVerified())return Fail(error,"Recurrent Q8 preflight needs its verified live parent.");
  unsigned count=0;std::array<uint8_t,4096> page={};
  for(unsigned i=0;i<parts.size();i++)if(proposal.parts[i].encoding==Plan::RecurrentQ8Bfp8){
   if(!ReadPage(i,0,page.data(),page.size(),error))return false;count++;
  }
  if(count!=1008 || !Current())return Lose(error,"Recurrent Q8 preflight coverage failed.");
  verified=true;if(!AllVerified())return Lose(error,"Recurrent Q8 source changed during preflight.");error.clear();return true;
 }
};
#endif
