/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent upload receipt is isolated offline inspection only
#endif
#ifndef PIONEER_Q38_SMALL_RECURRENT_UPLOAD_RECEIPT_INSPECTION_H
#define PIONEER_Q38_SMALL_RECURRENT_UPLOAD_RECEIPT_INSPECTION_H
#include "PioneerQ38SmallRecurrentLedgerInspection.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanController.h"

// Synthetic-owner receipt: physical-order page accounting only. FNV is not
// authentication or readback; this grants no native upload or launch.
class PioneerQ38SmallRecurrentUploadReceiptInspection {
 using C=PioneerQ38CanonicalModelPlan;using L=PioneerQ38SmallRecurrentKernelLayoutInspection;
 C original;L::Parent::Parent::Parent ancestor;L::Parent::Parent dense;L::Parent recurrent;L layout;
 unsigned order[3551]={};unsigned position=0,page=0,parts=0,spans=0,pages=0,pendingIndex=0,pendingPages=0,recorded=0;
 const void* context=nullptr;const void* cookie=nullptr;const uint32_t* hashes=nullptr;const uint8_t* ledger=nullptr;
 PioneerResidentBanks* banks=nullptr;const tt_dram_samples_result* proof=nullptr;const PioneerQ38SpanLease* lease=nullptr;
 int64_t team=-1;uint64_t generation=0,digest=UINT64_C(14695981039346656037);
 bool attempted=false,ready=false,pending=false,failed=false,sealed=false;
 bool Current()const{return ready && !failed && context && cookie && hashes && ledger && banks && proof && team>=0 && generation
  && original.OriginalFormatLayout() && layout.Ready() && banks->Matches(*proof);}
 bool Fail(){failed=true;if(banks)banks->Quarantine();return false;}
public:
 PioneerQ38SmallRecurrentUploadReceiptInspection()=default;
 PioneerQ38SmallRecurrentUploadReceiptInspection(const PioneerQ38SmallRecurrentUploadReceiptInspection&)=delete;
 PioneerQ38SmallRecurrentUploadReceiptInspection& operator=(const PioneerQ38SmallRecurrentUploadReceiptInspection&)=delete;
 bool Begin(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen,
  PioneerResidentBanks& bound,const tt_dram_samples_result& dram,unsigned ledgerPages){
  if(attempted)return false;attempted=true;
  if(!c || !fd || !h || !initialized || owner<0 || !gen || ledgerPages!=PioneerResidentBanks::Pages || !bound.Matches(dram))return false;
  for(unsigned i=0;i<ledgerPages;i++)if(initialized[i] || h[i])return false;
  if(!original.Build() || !ancestor.Build(original) || !dense.Build(original,ancestor)
   || !recurrent.Build(original,dense,ancestor) || !layout.Build(original,recurrent,dense,ancestor))return false;
  unsigned count=0;
  for(unsigned i=0;i<original.Count();i++)if(original.Part(i)->kind==C::WeightKind){
   if(count>=3551)return false;const auto* q=layout.At(i);if(!q)return false;unsigned at=count;
   while(at){const auto* prior=layout.At(order[at-1]);if(!prior)return false;
    if(prior->bank<q->bank || (prior->bank==q->bank && prior->offset<q->offset))break;
    order[at]=order[at-1];at--;
   }
   order[at]=i;count++;
  }
  if(count!=3551)return false;
  for(unsigned i=1;i<count;i++){const auto* a=layout.At(order[i-1]);const auto* b=layout.At(order[i]);
   if(!a || !b || a->bank>b->bank || (a->bank==b->bank && uint64_t(a->offset)+a->allocated>b->offset))return false;}
  context=c;cookie=fd;hashes=h;ledger=initialized;team=owner;generation=gen;banks=&bound;proof=&dram;ready=true;return true;
 }
 bool Next(unsigned& bank,uint64_t& offset,unsigned& length)const{
  if(!Current() || pending || sealed || position>=3551)return false;
  const auto* q=layout.At(order[position]);if(!q || page>=q->allocated/4096)return false;
  const unsigned remain=q->allocated/4096-page,n=remain<256?remain:256;
  bank=q->bank;offset=uint64_t(q->offset)+uint64_t(page)*4096;length=n*4096;return true;
 }
 bool Before(const void* c,const uint32_t* h,const uint8_t* initialized,const PioneerQ38SpanLease* span,
  unsigned bank,uint64_t offset,unsigned length){
  unsigned wantBank=0,wantBytes=0;uint64_t wantOffset=0;
  if(c!=context || h!=hashes || initialized!=ledger || !span || (lease && lease!=span)
   || span->closed || span->unsafe || span->running || span->nonce!=spans
   || !Next(wantBank,wantOffset,wantBytes) || bank!=wantBank || offset!=wantOffset || length!=wantBytes)return false;
  const uint64_t index=uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096;
  if(index>=PioneerResidentBanks::Pages || length/4096>PioneerResidentBanks::Pages-index)return false;
  for(unsigned i=0;i<length/4096;i++)if(ledger[index+i] || hashes[index+i])return false;
  lease=span;pendingIndex=uint32_t(index);pendingPages=length/4096;recorded=0;pending=true;return true;
 }
 bool RecordPage(unsigned index,uint32_t hash){
  if(!Current() || !pending || recorded>=pendingPages || index!=pendingIndex+recorded || ledger[index]!=1 || hashes[index]!=hash)return Fail();
  digest=(digest^index)*UINT64_C(1099511628211);digest=(digest^hash)*UINT64_C(1099511628211);recorded++;return true;
 }
 bool After(){
  if(!Current() || !pending || recorded!=pendingPages || !lease || lease->nonce!=spans+1
   || !lease->saved || !lease->ready || !lease->dirty || lease->running || lease->closed || lease->unsafe)return Fail();
  const auto* q=layout.At(order[position]);if(!q || pendingPages>q->allocated/4096-page)return Fail();
  page+=pendingPages;pages+=pendingPages;spans++;pending=false;
  if(page==q->allocated/4096){page=0;position++;parts++;}return true;
 }
 void Abort(){if(pending)Fail();}
 void Invalidate(){Fail();}
 bool Seal(const PioneerQ38SpanLease& span){
  if(!Current() || pending || sealed || &span!=lease || position!=3551 || page || parts!=3551 || spans!=21475 || pages!=5067109
   || !span.closed || !span.saved || span.nonce!=spans || span.dirty || span.ready || span.running || span.unsafe)return false;
  if(!PioneerQ38SmallRecurrentLedgerInspection::Verify(original,layout,ledger,hashes,PioneerResidentBanks::Pages,digest))return Fail();
  sealed=true;return true;
 }
 bool Sealed()const{return sealed && Current() && !pending && lease && lease->closed && lease->saved
  && lease->nonce==spans && !lease->dirty && !lease->ready && !lease->running && !lease->unsafe;}
 unsigned Spans()const{return spans;}
 bool Revalidate(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen){
  if(!Sealed() || context!=c || cookie!=fd || hashes!=h || ledger!=initialized || team!=owner || generation!=gen)return false;
  return PioneerQ38SmallRecurrentLedgerInspection::Verify(original,layout,ledger,hashes,PioneerResidentBanks::Pages,digest) || Fail();
 }
};
static_assert(sizeof(PioneerQ38SmallRecurrentUploadReceiptInspection)>200000,"receipt is HEAP ONLY");
#endif
