/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent prepared pages are isolated offline data only
#endif
#ifndef PIONEER_Q38_HYBRID_SMALL_RECURRENT_PREPARED_PAGES_H
#define PIONEER_Q38_HYBRID_SMALL_RECURRENT_PREPARED_PAGES_H
#include "PioneerQ38HybridSmallRecurrentBfp8Plan.h"
#include "PioneerQ38HybridRecurrentQ8PreparedPages.h"

// Prepared data routing only: no device owner, upload or execution authority.
template<class Source,class PackedPlan,class Bfp8,class ParentPages>
class PioneerQ38HybridSmallRecurrentPreparedPages {
 using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridSmallRecurrentBfp8Plan;
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
 PioneerQ38HybridSmallRecurrentPreparedPages()=default;
 PioneerQ38HybridSmallRecurrentPreparedPages(const PioneerQ38HybridSmallRecurrentPreparedPages&)=delete;
 PioneerQ38HybridSmallRecurrentPreparedPages& operator=(const PioneerQ38HybridSmallRecurrentPreparedPages&)=delete;
 bool Bind(Source& s,const PackedPlan& p,const Canonical& c,Bfp8& b,ParentPages& base,const Cache::Digest& sha,std::string& error){
  if(attempted)return Fail(error,"Small recurrent page binding is one-shot.");attempted=true;
  if(!base.MatchesBinding(s,p,c,sha) || !base.AllVerified() || !b.MatchesIdentity(s,sha))
   return Fail(error,"Small recurrent pages require the verified parent and same BFP8 source.");
  Plan::Proposal staged,prior;if(!Plan::Build(c,staged) || !Plan::Parent::Build(c,prior)
   || staged.parts.size()!=p.Parts().size() || prior.parts.size()!=staged.parts.size())return Fail(error,"Small recurrent geometry differs.");
  auto original=p.Parts();
  for(unsigned i=0;i<original.size();i++){
   const auto* old=base.Part(i);const auto* logical=c.Part(i);const auto& part=original[i];const auto& expected=prior.parts[i];
   if(!old || !logical || old->original!=expected.original || old->bank!=expected.bank || old->offset!=expected.offset
    || old->bytes!=expected.bytes || old->allocated!=expected.allocated || old->encoding!=expected.encoding
    || part.kind!=logical->kind || part.layer!=logical->layer || part.role!=logical->role || part.bank!=logical->bank
    || part.firstRow!=logical->firstRow || part.rows!=logical->rows || part.rowBytes!=logical->rowBytes
    || part.bytes!=logical->bytes || part.allocated!=logical->allocated
    || part.proposedAddress!=Canonical::Base+logical->offset || staged.parts[i].original!=i)
    return Fail(error,"Small recurrent original or parent snapshot differs.");
  }
  if(!base.MatchesVerifiedSnapshot(original))return Fail(error,"Small recurrent parent snapshot differs.");
  source=&s;packed=&p;canonical=&c;eight=&b;parent=&base;digest=sha;proposal=std::move(staged);parts=std::move(original);ready=true;
  if(!Current())return Lose(error,"Small recurrent source changed during binding.");error.clear();return true;
 }
 bool Ready()const{return Current();}
 bool AllVerified()const{return verified && Current() && parent->AllVerified() && Current();}
 const Base::Part* Part(unsigned i)const{return Current() && i<proposal.parts.size()?&proposal.parts[i]:nullptr;}
 bool OutputSafe(const void* out,size_t bytes)const{
  return out && bytes && Current() && parent->OutputSafe(out,bytes) && !Overlap(out,bytes,eight,sizeof(*eight))
   && !Overlap(out,bytes,this,sizeof(*this)) && !Overlap(out,bytes,parts.data(),parts.size()*sizeof(parts[0]))
   && !Overlap(out,bytes,proposal.parts.data(),proposal.parts.size()*sizeof(proposal.parts[0]));
 }
 bool ReadPages(unsigned fragment,unsigned page,unsigned count,uint8_t* out,unsigned capacity,std::string& error){
  if(!count || count>16 || !out || capacity<count*4096 || fragment>=proposal.parts.size())return Fail(error,"Invalid small recurrent output or fragment.");
  if(!Current())return Lose(error,"Small recurrent source changed.");
  if(!OutputSafe(out,size_t(count)*4096))return Fail(error,"Small recurrent output overlaps source state.");
  const auto part=packed->Parts()[fragment];const auto dest=proposal.parts[fragment];
  if(!Same(part,parts[fragment]))return Lose(error,"Small recurrent original part changed.");
  if(part.kind!=Canonical::WeightKind || page>=dest.allocated/4096 || count>dest.allocated/4096-page)
   return Fail(error,"Small recurrent page range is invalid.");
  std::vector<uint8_t> stage(count*4096,0);
  if(dest.encoding==Plan::SmallRecurrentBfp8){
   if(!eight->ReadPaddedSmallPages(part.tensor,page,count,stage.data(),unsigned(stage.size()),error))
    return Lose(error,"Prepared small recurrent BFP8 tensor unavailable.");
  }else if(!parent->ReadPages(fragment,page,count,stage.data(),unsigned(stage.size()),error))
   return Lose(error,"Parent recurrent Q8 page unavailable.");
  if(!Current() || !Same(packed->Parts()[fragment],part))return Lose(error,"Small recurrent source changed during read.");
  std::memcpy(out,stage.data(),stage.size());error.clear();return true;
 }
 bool ReadPage(unsigned fragment,unsigned page,uint8_t* out,unsigned capacity,std::string& error){return ReadPages(fragment,page,1,out,capacity,error);}
 bool VerifyAll(std::string& error){
  if(!Current() || verified || !parent->AllVerified())return Fail(error,"Small recurrent preflight requires verified live parent.");
  unsigned count=0;std::array<uint8_t,4096> page={};
  for(unsigned i=0;i<parts.size();i++)if(proposal.parts[i].encoding==Plan::SmallRecurrentBfp8){
   if(!ReadPage(i,0,page.data(),page.size(),error))return false;count++;
  }
  if(count!=96 || !Current())return Lose(error,"Small recurrent preflight coverage failed.");
  verified=true;if(!AllVerified())return Lose(error,"Small recurrent source changed during preflight.");error.clear();return true;
 }
};
#endif
