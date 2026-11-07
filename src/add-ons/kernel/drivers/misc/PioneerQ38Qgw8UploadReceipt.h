/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_UPLOAD_RECEIPT_H
#define PIONEER_Q38_QGW8_UPLOAD_RECEIPT_H
#include <stddef.h>
#include "PioneerQ38Qgw8KernelLayout.h"
#include "PioneerQ38SpanController.h"

// INSPECTION-ONLY kernel receipt for the distinct QGW8 extent sequence.
// It binds a fixed format/layout declaration, FD/team/generation, bank proof
// and host upload ledger. The FNV page digest is NOT cryptographic model
// authentication, proof of conversion correctness, or device readback.
// Nothing here enables a driver, Work, owner acquisition, or execution.
class PioneerQ38Qgw8UploadReceipt {
 PioneerQ38CanonicalModelPlan original;PioneerQ38Qgw8KernelLayout layout;
 const void* context=nullptr;const void* cookie=nullptr;
 const uint32_t* hashes=nullptr;const uint8_t* ledger=nullptr;
 PioneerResidentBanks* banks=nullptr;const tt_dram_samples_result* proof=nullptr;
 const PioneerQ38SpanLease* lease=nullptr;
 int64_t team=-1;uint64_t generation=0,bytes=0,digest=UINT64_C(14695981039346656037);
 unsigned part=0,page=0,parts=0,spans=0,pages=0,pendingPages=0,pendingIndex=0,recorded=0;
 bool attempted=false,ready=false,pending=false,failed=false,sealed=false;
 bool Current()const{return ready && !failed && context && cookie && hashes && ledger && banks && proof
  && original.OriginalFormatLayout() && layout.Ready() && team>=0 && generation && banks->Matches(*proof);}
 bool Fail(){failed=true;if(banks)banks->Quarantine();return false;}
 void Advance(){while(part<original.Count() && original.Part(part)->kind!=PioneerQ38CanonicalModelPlan::WeightKind)part++;}
 bool LedgerMatches()const{
  uint64_t check=UINT64_C(14695981039346656037);unsigned end[7]={};
  for(unsigned i=0;i<original.Count();i++){
   const auto* p=original.Part(i);const auto* q=layout.At(i);if(!p || !q || q->original!=i || p->bank!=q->bank)return false;
   if(p->kind!=PioneerQ38CanonicalModelPlan::WeightKind)continue;
   if(q->bank>=7 || q->offset/4096!=end[q->bank])return false;
   const unsigned first=q->bank*unsigned(PioneerResidentBanks::Capacity/4096)+q->offset/4096;
   for(unsigned n=0;n<q->allocated/4096;n++){
    const unsigned index=first+n;if(index>=PioneerResidentBanks::Pages || ledger[index]!=1)return false;
    check=(check^index)*UINT64_C(1099511628211);check=(check^hashes[index])*UINT64_C(1099511628211);
   }
   end[q->bank]+=q->allocated/4096;
  }
  if(check!=digest)return false;
  for(unsigned b=0;b<7;b++)for(unsigned p=end[b];p<PioneerResidentBanks::Capacity/4096;p++){
   const unsigned index=b*unsigned(PioneerResidentBanks::Capacity/4096)+p;
   if(ledger[index] || hashes[index])return false;
  }
  return true;
 }
public:
 PioneerQ38Qgw8UploadReceipt()=default;
 PioneerQ38Qgw8UploadReceipt(const PioneerQ38Qgw8UploadReceipt&)=delete;
 PioneerQ38Qgw8UploadReceipt& operator=(const PioneerQ38Qgw8UploadReceipt&)=delete;
 bool Begin(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen,
  PioneerResidentBanks& bound,const tt_dram_samples_result& dram,unsigned ledgerPages){
  if(attempted)return false;attempted=true;
  if(!c || !fd || !h || !initialized || owner<0 || !gen || ledgerPages!=PioneerResidentBanks::Pages || !bound.Matches(dram))return false;
  for(unsigned i=0;i<ledgerPages;i++)if(initialized[i] || h[i])return false;
  if(!original.Build() || !layout.Build(original) || layout.Count()!=4678)return false;
  context=c;cookie=fd;hashes=h;ledger=initialized;team=owner;generation=gen;banks=&bound;proof=&dram;ready=true;Advance();return true;
 }
 bool Matches(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen)const{
  return Sealed() && context==c && cookie==fd && hashes==h && ledger==initialized && team==owner && generation==gen;
 }
 bool Next(unsigned& bank,uint64_t& offset,unsigned& length)const{
  if(!Current() || pending || sealed || part>=original.Count())return false;
  const auto* p=original.Part(part);const auto* q=layout.At(part);
  if(!p || !q || p->kind!=PioneerQ38CanonicalModelPlan::WeightKind || page>=q->allocated/4096)return false;
  const unsigned remaining=q->allocated/4096-page,n=remaining<256?remaining:256;
  bank=q->bank;offset=uint64_t(q->offset)+uint64_t(page)*4096;length=n*4096;return true;
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
 bool RecordPage(unsigned index,uint32_t hash){
  if(!ready || failed || !pending || recorded>=pendingPages || index!=pendingIndex+recorded
   || ledger[index]!=1 || hashes[index]!=hash)return Fail();
  digest=(digest^index)*UINT64_C(1099511628211);digest=(digest^hash)*UINT64_C(1099511628211);recorded++;return true;
 }
 bool After(){
  if(!Current() || !pending || recorded!=pendingPages || !lease || lease->nonce!=spans+1
   || !lease->saved || !lease->ready || !lease->dirty || lease->running || lease->closed || lease->unsafe)return Fail();
  const auto* q=layout.At(part);if(!q || pendingPages>q->allocated/4096-page)return Fail();
  page+=pendingPages;pages+=pendingPages;bytes+=uint64_t(pendingPages)*4096;spans++;pending=false;
  if(page==q->allocated/4096){page=0;part++;parts++;Advance();}
  return true;
 }
 void Abort(){if(pending)Fail();}
 void Invalidate(){Fail();}
 bool Seal(const PioneerQ38SpanLease& span){
  if(!Current() || pending || sealed || &span!=lease || part!=original.Count() || page || parts!=3551 || spans!=22035
   || pages!=5167877 || bytes!=UINT64_C(21167624192) || !span.closed || !span.saved || span.nonce!=spans || span.dirty
   || span.ready || span.running || span.unsafe)return false;
  if(!LedgerMatches())return Fail();
  sealed=true;return true;
 }
 bool Sealed()const{return sealed && Current() && !pending && lease && lease->closed && lease->saved
  && lease->nonce==spans && !lease->dirty && !lease->ready && !lease->running && !lease->unsafe;}
 const PioneerQ38Qgw8KernelLayout* Layout()const{return Sealed()?&layout:nullptr;}
 bool DeriveBoundLayout(PioneerQ38Qgw8KernelLayout& output,const void* c,const void* fd,const uint32_t* h,
  const uint8_t* initialized,int64_t owner,uint64_t gen){
  const uintptr_t target=reinterpret_cast<uintptr_t>(&output),self=reinterpret_cast<uintptr_t>(this);
  if(!Matches(c,fd,h,initialized,owner,gen) || target>UINTPTR_MAX-sizeof(output) || self>UINTPTR_MAX-sizeof(*this)
   || (target<self+sizeof(*this) && self<target+sizeof(output)))return false;
  if(!LedgerMatches())return Fail();
  if(!output.Build(original) || output.Count()!=layout.Count())return false;
  for(unsigned b=0;b<7;b++)if(output.Used(b)!=layout.Used(b))return false;
  for(unsigned i=0;i<layout.Count();i++){
   const auto* a=layout.At(i);const auto* b=output.At(i);
   if(!a || !b || a->original!=b->original || a->bank!=b->bank || a->offset!=b->offset
    || a->bytes!=b->bytes || a->allocated!=b->allocated || a->encoding!=b->encoding)return false;
  }
  return true;
 }
 // Build the runtime-shaped geometry from this same sealed receipt. The
 // converted head/FFN encoding still requires distinct worker selection;
 // geometry and the non-cryptographic upload ledger do not grant execution.
 bool DeriveBoundCanonical(PioneerQ38CanonicalModelPlan& output,const void* c,const void* fd,const uint32_t* h,
  const uint8_t* initialized,int64_t owner,uint64_t gen){
  const uintptr_t target=reinterpret_cast<uintptr_t>(&output),self=reinterpret_cast<uintptr_t>(this);
  const uintptr_t hp=reinterpret_cast<uintptr_t>(h),lp=reinterpret_cast<uintptr_t>(initialized);
  const size_t hb=size_t(PioneerResidentBanks::Pages)*sizeof(*h),lb=PioneerResidentBanks::Pages;
  if(!Matches(c,fd,h,initialized,owner,gen) || target>UINTPTR_MAX-sizeof(output) || self>UINTPTR_MAX-sizeof(*this)
   || hp>UINTPTR_MAX-hb || lp>UINTPTR_MAX-lb || (target<self+sizeof(*this) && self<target+sizeof(output))
   || (target<hp+hb && hp<target+sizeof(output)) || (target<lp+lb && lp<target+sizeof(output)))return false;
  if(!LedgerMatches())return Fail();
  if(!output.BuildQgw8Inspection(layout) || !output.Qgw8InspectionLayout() || output.OriginalFormatLayout()
   || output.HybridFormatLayout() || output.Count()!=original.Count() || output.RecurrentCount()!=original.RecurrentCount())return false;
  for(unsigned b=0;b<7;b++)if(output.Used(b)!=layout.Used(b))return false;
  for(unsigned i=0;i<layout.Count();i++){
   const auto* a=original.Part(i);const auto* q=layout.At(i);const auto* b=output.Part(i);
   if(!a || !q || !b || a->kind!=b->kind || a->layer!=b->layer || a->role!=b->role || a->type!=b->type
    || a->bank!=b->bank || a->firstRow!=b->firstRow || a->rows!=b->rows || a->rowBytes!=b->rowBytes
    || q->bank!=b->bank || q->offset!=b->offset || q->bytes!=b->bytes || q->allocated!=b->allocated)return false;
  }
  if(!Matches(c,fd,h,initialized,owner,gen))return false;
  if(!LedgerMatches())return Fail();
  return true;
 }
 uint64_t PageDigest()const{return Sealed()?digest:0;}
 unsigned Spans()const{return spans;}
};
static_assert(sizeof(PioneerQ38Qgw8UploadReceipt)>393216,"QGW8 receipt is heap-only");
#endif
