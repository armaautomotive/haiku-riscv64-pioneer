/* SPDX-License-Identifier: MIT */
// Real prepared-page routing plus real span adapter, with fake caches and
// fake transport only. This is not model data, a device run or throughput.
#define PIONEER_Q38_DENSE_Q8_PREPARED_FIXTURE_ONLY 1
#define PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_DENSE_Q8_UPLOAD_INSPECTION 1
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_resident_banks_test.cpp"
#include "pioneer_q38_dense_q8_prepared_pages_test.cpp"
#include "../PioneerQ38HybridSpanUpload.h"
#include "../PioneerQ38DenseQ8UploadReceiptInspection.h"
#include "../PioneerQ38DenseQ8MatrixManifest.h"
struct FakeDenseIO {
 const PioneerQ38CanonicalModelPlan& canonical;DensePages& pages;
 PioneerQ38DenseQ8UploadReceiptInspection& receipt;PioneerQ38SpanLease& span;const void* context;
 std::vector<uint8_t>& ledger;std::vector<uint32_t>& hashes;
 unsigned calls=0,fragment=0,page=0,converted=0;uint64_t bytes=0;
#ifdef PIONEER_Q38_PIPELINED_UPLOAD_EXPERIMENT
 Plan::Proposal immutablePlan={};bool planReady=false;
#endif
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==42 && op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && size==sizeof(tt_q38_span_upload));
  while(fragment<canonical.Count() && canonical.Part(fragment)->kind!=canonical.WeightKind)fragment++;
  assert(fragment<canonical.Count());
#ifdef PIONEER_Q38_PIPELINED_UPLOAD_EXPERIMENT
  // Real driver calls do not inspect the userspace Pages object while the
  // producer prepares the next packet. Use a separate immutable fake plan.
  if(!planReady){assert(Plan::Build(canonical,immutablePlan));planReady=true;}
  const auto* p=&immutablePlan.parts[fragment];
#else
  const auto* p=pages.Part(fragment);
#endif
  assert(p);
  auto& q=*static_cast<tt_q38_span_upload*>(data);const unsigned n=std::min(256U,p->allocated/4096-page);
  assert(q.version==1 && !q.reserved && q.bank==p->bank && q.offset==uint64_t(p->offset)+uint64_t(page)*4096 && q.bytes==n*4096);
  const unsigned expected=p->encoding==Plan::DenseQ8Bfp8 || p->encoding==PioneerQ38HybridModelPlan::Bfp8?0x8b:
   p->encoding==PioneerQ38HybridModelPlan::Bfp4?0x4b:p->encoding==Plan::Parent::HeadBfp8?0x8d:0x5a;
  for(unsigned i=0;i<n;i++)assert(reinterpret_cast<const uint8_t*>(q.data)[i*4096]==expected);
  if(p->encoding==Plan::DenseQ8Bfp8){
   converted+=n;if(page+n==p->allocated/4096 && (p->bytes&4095))
    assert(reinterpret_cast<const uint8_t*>(q.data)[q.bytes-1]==0);
  }
  assert(receipt.Before(context,hashes.data(),ledger.data(),&span,q.bank,q.offset,q.bytes));
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
#ifndef PIONEER_Q38_DENSE_Q8_PREPARED_UPLOAD_FIXTURE_ONLY
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());FakeSource source;FakePlan packed;packed.source=&source;
 std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){
  const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){PioneerQ38Tensor t;if(!PioneerQ38Expected(layer,role,t))break;
   ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(t);}
 }
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);assert(p);const unsigned tensor=p->kind==canonical->WeightKind?ids.at({p->layer,p->role}):851;
  packed.parts.push_back({p->kind,p->layer,p->role,tensor,p->bank,p->firstRow,p->rows,p->rowBytes,canonical->Base+p->offset,p->bytes,p->allocated});
 }
 const auto digest=PioneerModelWeightCache::Hash("authenticated-fixture",21);
 const auto four=PioneerModelWeightCache::Hash("four-fingerprint",16),head=PioneerModelWeightCache::Hash("head-fingerprint",16);
 FakeBfp8 eight{&source};eight.digest=digest;std::string error;HeadPages parent;
 assert(parent.Bind(source,packed,*canonical,eight,digest,four,head,"/private/tmp/four","/private/tmp/head",[]{return true;},error));
 assert(parent.VerifyAll(error));DensePages pages;assert(pages.Bind(source,packed,*canonical,eight,parent,digest,error) && pages.VerifyAll(error));
 BanksMock mock;auto proof=BankProof(mock);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 auto receipt=std::make_unique<PioneerQ38DenseQ8UploadReceiptInspection>();PioneerQ38SpanLease span;int context=0,cookie=0;
 assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
 FakeDenseIO io{*canonical,pages,*receipt,span,&context,ledger,hashes};
 PioneerQ38HybridSpanUpload<DensePages,FakeDenseIO,PioneerQ38DenseQ8UploadPlan> upload;
 assert(upload.Run(pages,*canonical,io,42,error) && upload.CompleteHeld());
 assert(upload.UploadedParts()==3551 && upload.UploadedBytes()==io.bytes && io.calls==upload.UploadedSpans() && io.converted>0);
 span.saved=true;span.closed=true;span.nonce=io.calls;span.ready=span.dirty=false;
 assert(receipt->Seal(span) && receipt->Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42));
 std::array<uint32_t,PioneerQ38DenseQ8UploadReceiptInspection::DenseManifestWords> manifest={},expected={};
 PioneerQ38HybridDenseQ8MatrixPlan::Proposal proposal;assert(PioneerQ38HybridDenseQ8MatrixPlan::Build(*canonical,proposal));
 assert(PioneerQ38DenseQ8MatrixManifest::Build(*canonical,proposal,expected.data(),expected.size()));
 assert(receipt->DenseManifest(manifest.data(),manifest.size(),&context,&cookie,hashes.data(),ledger.data(),17,42) && manifest==expected);
 printf("PASS prepared dense-Q8 pages through %u fake span calls, sealed receipt and matching ROOT manifest; %u converted pages; NO HARDWARE OR MODEL\n",io.calls,io.converted);
}
#endif
