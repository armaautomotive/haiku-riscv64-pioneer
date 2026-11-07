/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_UPLOAD_RECEIPT_H
#define PIONEER_Q38_HYBRID_UPLOAD_RECEIPT_H
#include <stddef.h>
#include "PioneerQ38CanonicalModelPlan.h"
#include "PioneerQ38SpanController.h"

// Kernel-owned, one-shot record of the exact hybrid upload sequence. The
// caller holds the real FD/team/global lock and must use the same context,
// bank proof, ledger and driver generation throughout. The ledger hashes
// are existing non-cryptographic transport bookkeeping: this class does NOT
// authenticate the model or establish that device bytes match the host.
class PioneerQ38HybridUploadReceipt {
 PioneerQ38CanonicalModelPlan plan;
 const void* context=nullptr;const void* cookie=nullptr;
 const uint32_t* hashes=nullptr;const uint8_t* ledger=nullptr;
 PioneerResidentBanks* banks=nullptr;const tt_dram_samples_result* proof=nullptr;
 const PioneerQ38SpanLease* lease=nullptr;
 int64_t team=-1;uint64_t generation=0,bytes=0,digest=UINT64_C(14695981039346656037);
 unsigned part=0,page=0,parts=0,spans=0,pages=0,pendingPages=0,pendingIndex=0,recorded=0;
 bool attempted=false,ready=false,pending=false,failed=false,sealed=false;
 bool Current()const{return ready && !failed && context && cookie && hashes && ledger && banks && proof
  && team>=0 && generation && banks->Matches(*proof);}
 bool Fail(){failed=true;if(banks)banks->Quarantine();return false;}
 void Advance(){while(part<plan.Count() && plan.Part(part)->kind!=PioneerQ38CanonicalModelPlan::WeightKind)part++;}
 bool LedgerMatches()const{
  uint64_t check=UINT64_C(14695981039346656037);unsigned end[7]={};
  for(unsigned i=0;i<plan.Count();i++){
   const auto* p=plan.Part(i);if(!p)return false;
   if(p->kind!=PioneerQ38CanonicalModelPlan::WeightKind)continue;
   if(p->bank>=7 || p->offset/4096!=end[p->bank])return false;
   const unsigned first=p->bank*unsigned(PioneerResidentBanks::Capacity/4096)+p->offset/4096;
   for(unsigned n=0;n<p->allocated/4096;n++){
    const unsigned index=first+n;if(index>=PioneerResidentBanks::Pages || ledger[index]!=1)return false;
    check=(check^index)*UINT64_C(1099511628211);check=(check^hashes[index])*UINT64_C(1099511628211);
   }
   end[p->bank]+=p->allocated/4096;
  }
  if(check!=digest)return false;
  for(unsigned b=0;b<7;b++)for(unsigned p=end[b];p<PioneerResidentBanks::Capacity/4096;p++){
   const unsigned index=b*unsigned(PioneerResidentBanks::Capacity/4096)+p;
   if(ledger[index] || hashes[index])return false;
  }
  return true;
 }
public:
 PioneerQ38HybridUploadReceipt()=default;
 PioneerQ38HybridUploadReceipt(const PioneerQ38HybridUploadReceipt&)=delete;
 PioneerQ38HybridUploadReceipt& operator=(const PioneerQ38HybridUploadReceipt&)=delete;
 bool Begin(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen,
  PioneerResidentBanks& bound,const tt_dram_samples_result& dram,unsigned ledgerPages){
  if(attempted)return false;
  attempted=true;
  if(!c || !fd || !h || !initialized || owner<0 || !gen || ledgerPages!=PioneerResidentBanks::Pages || !bound.Matches(dram))return false;
  for(unsigned i=0;i<ledgerPages;i++)if(initialized[i] || h[i])return false;
  if(!plan.BuildHybridLayout() || !plan.HybridFormatLayout() || plan.Count()!=4678)return false;
  context=c;cookie=fd;hashes=h;ledger=initialized;team=owner;generation=gen;banks=&bound;proof=&dram;ready=true;Advance();return true;
 }
 bool Matches(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen)const{
  return Sealed() && context==c && cookie==fd && hashes==h && ledger==initialized && team==owner && generation==gen;
 }
 // Exposed fixed expected extent, not a user-selected route or capability.
 bool Next(unsigned& bank,uint64_t& offset,unsigned& length)const{
  if(!Current() || pending || sealed || part>=plan.Count())return false;
  const auto* p=plan.Part(part);if(!p || p->kind!=PioneerQ38CanonicalModelPlan::WeightKind || page>=p->allocated/4096)return false;
  const unsigned remaining=p->allocated/4096-page,n=remaining<256?remaining:256;
  bank=p->bank;offset=uint64_t(p->offset)+uint64_t(page)*4096;length=n*4096;return true;
 }
 bool Before(const void* c,const uint32_t* h,const uint8_t* initialized,const PioneerQ38SpanLease* span,
  unsigned bank,uint64_t offset,unsigned length){
  unsigned expectedBank=0,expectedBytes=0;uint64_t expectedOffset=0;
  if(c!=context || h!=hashes || initialized!=ledger || !span || (lease && lease!=span)
   || span->closed || span->unsafe || span->running || span->nonce!=spans
   || !Next(expectedBank,expectedOffset,expectedBytes)
   || bank!=expectedBank || offset!=expectedOffset || length!=expectedBytes)return false;
  const uint64_t index=uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096;
  if(index>=PioneerResidentBanks::Pages || length/4096>PioneerResidentBanks::Pages-index)return false;
  for(unsigned i=0;i<length/4096;i++)if(ledger[index+i] || hashes[index+i])return false;
  lease=span;pendingIndex=uint32_t(index);pendingPages=length/4096;recorded=0;pending=true;return true;
 }
 // Called immediately after the real span service publishes each page hash.
 bool RecordPage(unsigned index,uint32_t hash){
  if(!ready || failed || !pending || recorded>=pendingPages || index!=pendingIndex+recorded
   || ledger[index]!=1 || hashes[index]!=hash)return Fail();
  digest=(digest^index)*UINT64_C(1099511628211);digest=(digest^hash)*UINT64_C(1099511628211);recorded++;return true;
 }
 bool After(){
  if(!Current() || !pending || recorded!=pendingPages || !lease || lease->nonce!=spans+1
   || !lease->saved || !lease->ready || !lease->dirty || lease->running || lease->closed || lease->unsafe)return Fail();
  const auto* p=plan.Part(part);if(!p || pendingPages>p->allocated/4096-page)return Fail();
  page+=pendingPages;pages+=pendingPages;bytes+=uint64_t(pendingPages)*4096;spans++;pending=false;
  if(page==p->allocated/4096){page=0;part++;parts++;Advance();}
  return true;
 }
 void Abort(){if(pending)Fail();} // uncertainty after preparation retains/quarantines the banks
 void Invalidate(){Fail();} // reply/lifetime uncertainty, never hardware IO or cleanup
 bool Seal(const PioneerQ38SpanLease& span){
  if(!Current() || pending || sealed || &span!=lease || part!=plan.Count() || page || parts!=3551 || spans!=22005
   || pages!=5160410 || bytes!=UINT64_C(21137039360) || !span.closed || !span.saved || span.nonce!=spans || span.dirty
   || span.ready || span.running || span.unsafe)return false;
  if(!LedgerMatches())return Fail();
  sealed=true;return true;
 }
 bool Sealed()const{return sealed && Current() && !pending && lease && lease->closed && lease->saved
  && lease->nonce==spans && !lease->dirty && !lease->ready && !lease->running && !lease->unsafe;}
 const PioneerQ38CanonicalModelPlan* Layout()const{return Sealed()?&plan:nullptr;}
 bool DeriveBoundLayout(PioneerQ38CanonicalModelPlan& output,const void* c,const void* fd,const uint32_t* h,
  const uint8_t* initialized,int64_t owner,uint64_t gen){
  const uintptr_t target=reinterpret_cast<uintptr_t>(&output),self=reinterpret_cast<uintptr_t>(this);
  if(!Matches(c,fd,h,initialized,owner,gen) || target>UINTPTR_MAX-sizeof(output) || self>UINTPTR_MAX-sizeof(*this)
   || (target<self+sizeof(*this) && self<target+sizeof(output)))return false;
  if(!LedgerMatches())return Fail();
  if(!output.BuildHybridLayout() || !output.HybridFormatLayout()
   || output.Count()!=plan.Count() || output.RecurrentCount()!=plan.RecurrentCount())return false;
  for(unsigned b=0;b<7;b++)if(output.Used(b)!=plan.Used(b))return false;
  for(unsigned i=0;i<plan.Count();i++){
   const auto* a=plan.Part(i);const auto* b=output.Part(i);
   if(!a || !b || a->kind!=b->kind || a->layer!=b->layer || a->role!=b->role || a->type!=b->type
    || a->bank!=b->bank || a->firstRow!=b->firstRow || a->rows!=b->rows || a->rowBytes!=b->rowBytes
    || a->offset!=b->offset || a->bytes!=b->bytes || a->allocated!=b->allocated)return false;
  }
  return true; // fixed layout and same owner,not source authentication or launch authority
 }
 uint64_t PageDigest()const{return Sealed()?digest:0;}
 unsigned Spans()const{return spans;}
};
static_assert(sizeof(PioneerQ38HybridUploadReceipt)>262144,"complete hybrid receipt is heap-only");
#endif
