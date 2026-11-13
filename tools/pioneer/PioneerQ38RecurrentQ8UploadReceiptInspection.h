/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent upload receipt is offline inspection only
#endif
#ifndef PIONEER_Q38_RECURRENT_Q8_UPLOAD_RECEIPT_INSPECTION_H
#define PIONEER_Q38_RECURRENT_Q8_UPLOAD_RECEIPT_INSPECTION_H
#include "PioneerQ38RecurrentQ8LedgerInspection.h"
#include "PioneerQ38RecurrentQ8MatrixWorkspace.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanController.h"

// CPU ledger experiment only: FNV is not authentication or card readback.
// A sealed receipt does not authorize a driver, workspace write or launch.
class PioneerQ38RecurrentQ8UploadReceiptInspection {
 using C=PioneerQ38CanonicalModelPlan;using L=PioneerQ38RecurrentQ8KernelLayoutInspection;
 C original;L::Parent::Parent ancestor;L::Parent parent;L layout;
 const void* context=nullptr;const void* cookie=nullptr;const uint32_t* hashes=nullptr;const uint8_t* ledger=nullptr;
 PioneerResidentBanks* banks=nullptr;const tt_dram_samples_result* proof=nullptr;const PioneerQ38SpanLease* lease=nullptr;
 int64_t team=-1;uint64_t generation=0,bytes=0,digest=UINT64_C(14695981039346656037);
 unsigned part=0,page=0,parts=0,spans=0,pages=0,pendingIndex=0,pendingPages=0,recorded=0;
 uint32_t denseManifestScratch[16*3*7*3]={}; // receipt is heap-only
 bool attempted=false,ready=false,pending=false,failed=false,sealed=false;
 bool Current()const{return ready && !failed && context && cookie && hashes && ledger && banks && proof
  && original.OriginalFormatLayout() && ancestor.Ready() && parent.Ready() && layout.Ready()
  && team>=0 && generation && banks->Matches(*proof);}
 bool Fail(){failed=true;if(banks)banks->Quarantine();return false;}
 void Advance(){while(part<original.Count() && original.Part(part)->kind!=C::WeightKind)part++;}
public:
 PioneerQ38RecurrentQ8UploadReceiptInspection()=default;
 PioneerQ38RecurrentQ8UploadReceiptInspection(const PioneerQ38RecurrentQ8UploadReceiptInspection&)=delete;
 PioneerQ38RecurrentQ8UploadReceiptInspection& operator=(const PioneerQ38RecurrentQ8UploadReceiptInspection&)=delete;
 bool Begin(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen,
  PioneerResidentBanks& bound,const tt_dram_samples_result& dram,unsigned ledgerPages){
  if(attempted)return false;attempted=true;
  if(!c || !fd || !h || !initialized || owner<0 || !gen || ledgerPages!=PioneerResidentBanks::Pages || !bound.Matches(dram))return false;
  for(unsigned i=0;i<ledgerPages;i++)if(initialized[i] || h[i])return false;
  if(!original.Build() || !ancestor.Build(original) || !parent.Build(original,ancestor) || !layout.Build(original,parent,ancestor))return false;
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
  if(!Current() || pending || sealed || &span!=lease || part!=original.Count() || page || parts!=3551 || spans!=21475
   || pages!=5065477 || bytes!=UINT64_C(20748193792) || !span.closed || !span.saved || span.nonce!=spans
   || span.dirty || span.ready || span.running || span.unsafe)return false;
  if(!PioneerQ38RecurrentQ8LedgerInspection::Verify(original,layout,ledger,hashes,PioneerResidentBanks::Pages,digest))return Fail();
  sealed=true;return true;
 }
 bool Sealed()const{return sealed && Current() && !pending && lease && lease->closed && lease->saved
  && lease->nonce==spans && !lease->dirty && !lease->ready && !lease->running && !lease->unsafe;}
 bool Matches(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen)const{
  return Sealed() && context==c && cookie==fd && hashes==h && ledger==initialized && team==owner && generation==gen;
 }
 bool MatchesLease(const PioneerQ38SpanLease* span)const{return span && span==lease && Sealed();}
 bool MatchesBanks(const PioneerResidentBanks* b,const tt_dram_samples_result* p)const{
  return b && p && banks==b && proof==p && Sealed();
 }
 bool Revalidate(const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,int64_t owner,uint64_t gen){
  if(!Matches(c,fd,h,initialized,owner,gen))return false;
  return PioneerQ38RecurrentQ8LedgerInspection::Verify(original,layout,ledger,hashes,PioneerResidentBanks::Pages,digest) || Fail();
 }
 // The startup view is the QGW8 logical ancestor, not the physical BFP8
 // geometry. Matching it does not assert that every logical page is present.
 bool MatchesLogical(const C& logical,const void* c,const void* fd,const uint32_t* h,
  const uint8_t* initialized,int64_t owner,uint64_t gen){
  if(!logical.Qgw8InspectionLayout() || logical.Count()!=original.Count()
   || logical.RecurrentCount()!=original.RecurrentCount() || !Revalidate(c,fd,h,initialized,owner,gen))return false;
  for(unsigned b=0;b<7;b++)if(logical.Used(b)!=ancestor.Used(b))return false;
  for(unsigned i=0;i<original.Count();i++){
   const auto* src=original.Part(i);const auto* dst=logical.Part(i);const auto* q=ancestor.At(i);
   if(!src || !dst || !q || src->kind!=dst->kind || src->layer!=dst->layer || src->role!=dst->role
    || src->type!=dst->type || src->firstRow!=dst->firstRow || src->rows!=dst->rows || src->rowBytes!=dst->rowBytes
    || q->bank!=dst->bank || q->offset!=dst->offset || q->bytes!=dst->bytes || q->allocated!=dst->allocated)return false;
  }
  return Revalidate(c,fd,h,initialized,owner,gen);
 }
 bool MatchesLogical(const C& logical,const void* c,const void* fd,const uint32_t* h,
  const uint8_t* initialized,int64_t owner,uint64_t gen,const PioneerResidentBanks& b,const tt_dram_samples_result& p){
  return MatchesBanks(&b,&p) && MatchesLogical(logical,c,fd,h,initialized,owner,gen);
 }
#ifdef PIONEER_Q38_RECURRENT_Q8_CANONICAL_INSPECTION
 // Construct one of the two distinct views only from this same sealed
 // ledger. This does not authenticate card bytes or grant execution.
 bool DeriveCanonical(C& output,bool physical,const void* c,const void* fd,const uint32_t* h,
  const uint8_t* initialized,int64_t owner,uint64_t gen){
  const uintptr_t out=reinterpret_cast<uintptr_t>(&output),self=reinterpret_cast<uintptr_t>(this),limit=~uintptr_t(0);
  const uintptr_t hp=reinterpret_cast<uintptr_t>(h),lp=reinterpret_cast<uintptr_t>(initialized);
  const uintptr_t cp=reinterpret_cast<uintptr_t>(c),fp=reinterpret_cast<uintptr_t>(fd);
  const size_t bytes=sizeof(output),hashBytes=size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t);
  if(out>limit-bytes || self>limit-sizeof(*this) || hp>limit-hashBytes || lp>limit-PioneerResidentBanks::Pages
   || (out<self+sizeof(*this) && self<out+bytes) || (out<hp+hashBytes && hp<out+bytes)
   || (out<lp+PioneerResidentBanks::Pages && lp<out+bytes)
   || (cp>=out && cp<out+bytes) || (fp>=out && fp<out+bytes)
   || !Revalidate(c,fd,h,initialized,owner,gen))return false;
  if(physical ? !output.BuildRecurrentQ8Inspection(parent,layout) : !output.BuildDenseQ8LogicalParentInspection(ancestor))return false;
  if(output.Count()!=original.Count() || output.RecurrentCount()!=original.RecurrentCount()
   || (physical ? !output.RecurrentQ8InspectionLayout() : !output.Qgw8InspectionLayout()))return false;
  for(unsigned b=0;b<7;b++)if(output.Used(b)!=(physical?layout.Used(b):ancestor.Used(b)))return false;
  for(unsigned i=0;i<original.Count();i++){
   const auto* src=original.Part(i);const auto* dst=output.Part(i);
   const auto* q=physical?layout.At(i):ancestor.At(i);
   if(!src || !dst || !q || src->kind!=dst->kind || src->layer!=dst->layer || src->role!=dst->role
    || src->type!=dst->type || src->firstRow!=dst->firstRow || src->rows!=dst->rows || src->rowBytes!=dst->rowBytes
    || q->bank!=dst->bank || q->offset!=dst->offset || q->bytes!=dst->bytes || q->allocated!=dst->allocated)return false;
  }
  return Revalidate(c,fd,h,initialized,owner,gen);
 }
#endif
 // Produce metadata only from this sealed ledger and independently rebuilt
 // layout. No write to card memory, readback or device ownership is implied.
 bool Workspace(uint32_t* output,unsigned words,const void* c,const void* fd,const uint32_t* h,
  const uint8_t* initialized,int64_t owner,uint64_t gen){
  using W=PioneerQ38RecurrentQ8MatrixWorkspace;using Plan=PioneerQ38HybridRecurrentQ8MatrixPlan;
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this);
  const uintptr_t hp=reinterpret_cast<uintptr_t>(h),lp=reinterpret_cast<uintptr_t>(initialized),limit=~uintptr_t(0);
  const uintptr_t cp=reinterpret_cast<uintptr_t>(c),fp=reinterpret_cast<uintptr_t>(fd);
  const size_t outputBytes=W::Bytes,hashBytes=size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t);
  if(!output || words!=W::Words || (out&3) || out>limit-outputBytes || self>limit-sizeof(*this)
   || hp>limit-hashBytes || lp>limit-PioneerResidentBanks::Pages
   || (out<self+sizeof(*this) && self<out+outputBytes) || (out<hp+hashBytes && hp<out+outputBytes)
   || (out<lp+PioneerResidentBanks::Pages && lp<out+outputBytes)
   || (cp>=out && cp<out+outputBytes) || (fp>=out && fp<out+outputBytes)
   || !Revalidate(c,fd,h,initialized,owner,gen))return false;
  Plan::Proposal proposal;if(!Plan::Build(original,proposal) || proposal.parts.size()!=layout.Count())return false;
  for(unsigned i=0;i<layout.Count();i++){
   const auto* q=layout.At(i);const auto& p=proposal.parts[i];
   if(!q || p.original!=q->original || p.bank!=q->bank || p.offset!=q->offset
    || p.bytes!=q->bytes || p.allocated!=q->allocated || p.encoding!=q->encoding)return Fail();
  }
  std::array<uint32_t,W::Words> staged={};
  if(!W::Build(original,proposal,staged.data(),staged.size()) || !Revalidate(c,fd,h,initialized,owner,gen))return false;
  for(unsigned i=0;i<W::Words;i++)output[i]=staged[i];return true;
 }
 enum : unsigned {DenseManifestWords=16*3*7*3};
 // Dense matrices remain in the same physical locations in the recurrent
 // layout. Derive their table from this receipt, not from a dense-only one.
 bool DenseManifest(uint32_t* output,unsigned words,const void* c,const void* fd,const uint32_t* h,
  const uint8_t* initialized,int64_t owner,uint64_t gen){
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),limit=~uintptr_t(0);
  const uintptr_t hp=reinterpret_cast<uintptr_t>(h),lp=reinterpret_cast<uintptr_t>(initialized);
  const uintptr_t cp=reinterpret_cast<uintptr_t>(c),fp=reinterpret_cast<uintptr_t>(fd);
  const size_t bytes=DenseManifestWords*sizeof(uint32_t),hashBytes=size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t);
  if(!output || words!=DenseManifestWords || (out&3) || out>limit-bytes || self>limit-sizeof(*this)
   || hp>limit-hashBytes || lp>limit-PioneerResidentBanks::Pages
   || (out<self+sizeof(*this) && self<out+bytes) || (out<hp+hashBytes && hp<out+bytes)
   || (out<lp+PioneerResidentBanks::Pages && lp<out+bytes)
   || (cp>=out && cp<out+bytes) || (fp>=out && fp<out+bytes)
   || !Revalidate(c,fd,h,initialized,owner,gen))return false;
  for(unsigned i=0;i<DenseManifestWords;i++)denseManifestScratch[i]=0;unsigned count=0;
  for(unsigned i=0;i<original.Count();i++){
   const auto* p=original.Part(i);const auto* q=layout.At(i);const auto* d=parent.At(i);if(!p || !q || !d)return Fail();
   if(p->kind!=C::WeightKind || p->layer>=64 || p->layer%4!=3 || p->role<5 || p->role>7)continue;
   if(p->bank>=7 || q->bank!=p->bank || q->offset!=d->offset || q->bytes!=d->bytes || q->allocated!=d->allocated
    || q->encoding!=L::Parent::DenseQ8Bfp8 || q->offset>UINT32_MAX-C::Base || !p->rows)return Fail();
   const unsigned at=(((p->layer/4)*3+p->role-5)*7+p->bank)*3;
   if(at>=DenseManifestWords || denseManifestScratch[at])return Fail();
   denseManifestScratch[at]=C::Base+q->offset;denseManifestScratch[at+1]=p->firstRow;denseManifestScratch[at+2]=p->rows;count++;
  }
  if(count!=16*3*7 || !Revalidate(c,fd,h,initialized,owner,gen))return false;
  for(unsigned i=0;i<DenseManifestWords;i++)output[i]=denseManifestScratch[i];return true;
 }
 uint64_t PageDigest()const{return Sealed()?digest:0;}
};
static_assert(sizeof(PioneerQ38RecurrentQ8UploadReceiptInspection)>524288,"recurrent receipt is heap-only");
#endif
