/* SPDX-License-Identifier: MIT */
// Actual recurrent page router and span adapter, simulated transport only.
#define PIONEER_Q38_RECURRENT_Q8_PREPARED_FIXTURE_ONLY 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_UPLOAD_INSPECTION 1
#define PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_DENSE_Q8_UPLOAD_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38RecurrentQ8MatrixWorkspace.h"
#include "pioneer_resident_banks_test.cpp"
#include "pioneer_q38_recurrent_q8_prepared_pages_test.cpp"
#include "../PioneerQ38HybridSpanUpload.h"
#include "../PioneerQ38RecurrentQ8UploadProtocolInspection.h"
#include "../PioneerQ38RecurrentQ8UploadReceiptInspection.h"
struct FakeRecurrentIO {
 const PioneerQ38CanonicalModelPlan& canonical;RecurrentPages& pages;
 PioneerQ38RecurrentQ8UploadReceiptInspection& receipt;PioneerQ38SpanLease& span;const void* context;
 std::vector<uint8_t>& ledger;std::vector<uint32_t>& hashes;
 unsigned calls=0,fragment=0,page=0,converted=0,failAt=0;uint64_t bytes=0;
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==42 && op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && size==sizeof(tt_q38_span_upload));
  while(fragment<canonical.Count() && canonical.Part(fragment)->kind!=canonical.WeightKind)fragment++;
  assert(fragment<canonical.Count());const auto* p=pages.Part(fragment);assert(p);
  auto& q=*static_cast<tt_q38_span_upload*>(data);const unsigned n=std::min(256U,p->allocated/4096-page);
  assert(q.version==1 && !q.reserved && q.bank==p->bank && q.offset==uint64_t(p->offset)+uint64_t(page)*4096 && q.bytes==n*4096);
  const unsigned expected=p->encoding==RecurrentPlan::RecurrentQ8Bfp8 || p->encoding==RecurrentPlan::Parent::DenseQ8Bfp8
   || p->encoding==PioneerQ38HybridModelPlan::Bfp8?0x8b:p->encoding==PioneerQ38HybridModelPlan::Bfp4?0x4b:
   p->encoding==RecurrentPlan::Parent::Parent::HeadBfp8?0x8d:0x5a;
  for(unsigned i=0;i<n;i++)for(unsigned within:{0U,31U,2048U,4095U}){
   const uint64_t position=uint64_t(page+i)*4096+within;
   assert(reinterpret_cast<const uint8_t*>(q.data)[i*4096+within]==(expected==0x8b && position>=p->bytes?0U:expected));
  }
  if(p->encoding==RecurrentPlan::RecurrentQ8Bfp8)converted+=n;
  assert(receipt.Before(context,hashes.data(),ledger.data(),&span,q.bank,q.offset,q.bytes));
  if(failAt && calls+1==failAt){calls++;receipt.Abort();return false;}
  const unsigned first=unsigned(uint64_t(q.bank)*(PioneerResidentBanks::Capacity/4096)+q.offset/4096);
  for(unsigned i=0;i<n;i++){
   const unsigned index=first+i;const uint32_t value=index*2654435761U+reinterpret_cast<const uint8_t*>(q.data)[i*4096];
   ledger[index]=1;hashes[index]=value;assert(receipt.RecordPage(index,value));
  }
  span.saved=span.ready=span.dirty=true;span.nonce=calls+1;assert(receipt.After());
  calls++;bytes+=q.bytes;page+=n;if(page==p->allocated/4096){fragment++;page=0;}
  q.result={1,TT_RUNTIME_PASSED,q.bytes,n,calls,1,1,1,0,0,0,0};return true;
 }
};
int main(){
 {
  namespace P=PioneerQ38RecurrentQ8UploadInspection;P::Packet p;p.operation=P::Begin;assert(P::Request(p));
  p.operation=P::Seal;assert(P::Request(p));p.magic=0x44554851;assert(!P::Request(p));p.magic=P::Magic;
  p.reserved=1;assert(!P::Request(p));p.reserved=0;p.accepted=1;assert(!P::Request(p));
  p.accepted=0;p.sealed=1;assert(!P::Request(p));p.sealed=0;p.spans=1;assert(!P::Request(p));
  p.spans=0;p.recoveryRequired=1;assert(!P::Request(p));p.recoveryRequired=0;p.operation=0;assert(!P::Request(p));
 }
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());FakeSource source;FakePlan packed;packed.source=&source;
 std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){PioneerQ38Tensor t;if(!PioneerQ38Expected(layer,role,t))break;
   ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(t);}}
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);assert(p);const unsigned tensor=p->kind==canonical->WeightKind?ids.at({p->layer,p->role}):851;
  packed.parts.push_back({p->kind,p->layer,p->role,tensor,p->bank,p->firstRow,p->rows,p->rowBytes,canonical->Base+p->offset,p->bytes,p->allocated});
 }
 const auto digest=PioneerModelWeightCache::Hash("authenticated-fixture",21);
 const auto four=PioneerModelWeightCache::Hash("four-fingerprint",16),headHash=PioneerModelWeightCache::Hash("head-fingerprint",16);
 FakeBfp8 parentEight{&source},recurrentEight{&source};parentEight.digest=recurrentEight.digest=digest;
 std::string error;HeadPages head;assert(head.Bind(source,packed,*canonical,parentEight,digest,four,headHash,"/private/tmp/four","/private/tmp/head",[]{return true;},error));
 assert(head.VerifyAll(error));DensePages parent;assert(parent.Bind(source,packed,*canonical,parentEight,head,digest,error) && parent.VerifyAll(error));
 RecurrentPages pages;assert(pages.Bind(source,packed,*canonical,recurrentEight,parent,digest,error) && pages.VerifyAll(error));
 BanksMock mock;auto proof=BankProof(mock);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 {
  BanksMock failedMock;auto failedProof=BankProof(failedMock);PioneerResidentBanks failedBanks;assert(failedBanks.Bind(failedProof));
  auto failedReceipt=std::make_unique<PioneerQ38RecurrentQ8UploadReceiptInspection>();PioneerQ38SpanLease failedSpan;int failedContext=0,failedCookie=0;
  assert(failedReceipt->Begin(&failedContext,&failedCookie,hashes.data(),ledger.data(),17,42,failedBanks,failedProof,ledger.size()));
  FakeRecurrentIO failed{*canonical,pages,*failedReceipt,failedSpan,&failedContext,ledger,hashes};failed.failAt=1;
  PioneerQ38HybridSpanUpload<RecurrentPages,FakeRecurrentIO,PioneerQ38RecurrentQ8UploadPlan> refused;
  assert(!refused.Run(pages,*canonical,failed,42,error) && failedBanks.Quarantined() && !refused.CompleteHeld());
  const unsigned calls=failed.calls;assert(!refused.Run(pages,*canonical,failed,42,error) && failed.calls==calls);
 }
 auto receipt=std::make_unique<PioneerQ38RecurrentQ8UploadReceiptInspection>();PioneerQ38SpanLease span;int context=0,cookie=0;
 assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
 FakeRecurrentIO io{*canonical,pages,*receipt,span,&context,ledger,hashes};
 PioneerQ38HybridSpanUpload<RecurrentPages,FakeRecurrentIO,PioneerQ38RecurrentQ8UploadPlan> upload;
 assert(upload.Run(pages,*canonical,io,42,error) && upload.CompleteHeld());
 assert(upload.UploadedParts()==3551 && upload.UploadedBytes()==UINT64_C(20748193792) && io.calls==21475 && io.converted==1436160);
 span.saved=true;span.closed=true;span.nonce=io.calls;span.ready=span.dirty=false;
 assert(receipt->Seal(span) && receipt->Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42));
 printf("PASS recurrent prepared pages through %u simulated spans and sealed ledger; %u recurrent converted pages; NO HARDWARE OR MODEL\n",io.calls,io.converted);
}
