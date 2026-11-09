/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION_H
#define PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION_H
#if !defined(PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION) || PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Dense Q8 upload receipt is isolated inspection only
#endif
#include "PioneerQ38DenseQ8LedgerInspection.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanController.h"

// Isolated owner/ledger experiment for the address-preserving QGWD format.
// The caller remains responsible for authenticated bytes, real FD/clock,
// device mappings, readback and launch. FNV detects ledger inconsistencies;
// it is not cryptographic authentication. Never pass this to a live driver.
class PioneerQ38DenseQ8UploadReceiptInspection {
 using C=PioneerQ38CanonicalModelPlan;using L=PioneerQ38DenseQ8KernelLayoutInspection;
 C original;L::Parent parent;L layout;
 const void* context=nullptr;const void* cookie=nullptr;const uint32_t* hashes=nullptr;const uint8_t* ledger=nullptr;
 PioneerResidentBanks* banks=nullptr;const tt_dram_samples_result* proof=nullptr;const PioneerQ38SpanLease* lease=nullptr;
 int64_t team=-1;uint64_t generation=0,bytes=0,digest=UINT64_C(14695981039346656037);
 unsigned part=0,page=0,parts=0,spans=0,pages=0,pendingIndex=0,pendingPages=0,recorded=0;
 bool attempted=false,ready=false,pending=false,failed=false,sealed=false;
 bool Current()const{return ready && !failed && context && cookie && hashes && ledger && banks && proof
  && original.OriginalFormatLayout() && parent.Ready() && layout.Ready() && team>=0 && generation && banks->Matches(*proof);}
 bool Fail(){failed=true;if(banks)banks->Quarantine();return false;}
 void Advance(){while(part<original.Count() && original.Part(part)->kind!=C::WeightKind)part++;}
public:
 PioneerQ38DenseQ8UploadReceiptInspection()=default;
 PioneerQ38DenseQ8UploadReceiptInspection(const PioneerQ38DenseQ8UploadReceiptInspection&)=delete;
 PioneerQ38DenseQ8UploadReceiptInspection& operator=(const PioneerQ38DenseQ8UploadReceiptInspection&)=delete;
 bool Begin(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen,
  PioneerResidentBanks& bound,const tt_dram_samples_result& dram,unsigned ledgerPages){
  if(attempted)return false;attempted=true;
  if(!c || !fd || !h || !initialized || owner<0 || !gen || ledgerPages!=PioneerResidentBanks::Pages || !bound.Matches(dram))return false;
  for(unsigned i=0;i<ledgerPages;i++)if(initialized[i] || h[i])return false;
  if(!original.Build() || !parent.Build(original) || !layout.Build(original,parent))return false;
  context=c;cookie=fd;hashes=h;ledger=initialized;team=owner;generation=gen;banks=&bound;proof=&dram;ready=true;Advance();return true;
 }
 bool Next(unsigned& bank,uint64_t& offset,unsigned& length)const{
  if(!Current() || pending || sealed || part>=original.Count())return false;
  const auto* p=original.Part(part);const auto* q=layout.At(part);
  if(!p || !q || p->kind!=C::WeightKind || page>=q->allocated/4096)return false;
  const unsigned remaining=q->allocated/4096-page,n=remaining<256?remaining:256;
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
  if(!Current() || !pending || recorded>=pendingPages || index!=pendingIndex+recorded
   || ledger[index]!=1 || hashes[index]!=hash)return Fail();
  digest=(digest^index)*UINT64_C(1099511628211);digest=(digest^hash)*UINT64_C(1099511628211);recorded++;return true;
 }
 bool After(){
  if(!Current() || !pending || recorded!=pendingPages || !lease || lease->nonce!=spans+1
   || !lease->saved || !lease->ready || !lease->dirty || lease->running || lease->closed || lease->unsafe)return Fail();
  const auto* q=layout.At(part);if(!q || pendingPages>q->allocated/4096-page)return Fail();
  page+=pendingPages;pages+=pendingPages;bytes+=uint64_t(pendingPages)*4096;spans++;pending=false;
  if(page==q->allocated/4096){page=0;part++;parts++;Advance();}return true;
 }
 void Abort(){if(pending)Fail();}
 bool Seal(const PioneerQ38SpanLease& span){
  if(!Current() || pending || sealed || &span!=lease || part!=original.Count() || page || parts!=3551 || spans!=21907
   || pages!=5149957 || bytes!=UINT64_C(21094223872) || !span.closed || !span.saved || span.nonce!=spans
   || span.dirty || span.ready || span.running || span.unsafe)return false;
  if(!PioneerQ38DenseQ8LedgerInspection::Verify(original,layout,ledger,hashes,PioneerResidentBanks::Pages,digest))return Fail();
  sealed=true;return true;
 }
 bool Sealed()const{return sealed && Current() && !pending && lease && lease->closed && lease->saved
  && lease->nonce==spans && !lease->dirty && !lease->ready && !lease->running && !lease->unsafe;}
 bool Revalidate(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen){
  if(!Sealed() || context!=c || cookie!=fd || hashes!=h || ledger!=initialized || team!=owner || generation!=gen)return false;
  return PioneerQ38DenseQ8LedgerInspection::Verify(original,layout,ledger,hashes,PioneerResidentBanks::Pages,digest) || Fail();
 }
 unsigned Spans()const{return spans;}
 uint64_t PageDigest()const{return Sealed()?digest:0;}
};
static_assert(sizeof(PioneerQ38DenseQ8UploadReceiptInspection)>524288,"dense-Q8 inspection receipt is heap-only");
#endif
