/* SPDX-License-Identifier: MIT */
// Real prepared-page routing plus real span adapter, with fake caches and
// fake transport only. This is not model data, a device run or throughput.
#define PIONEER_Q38_DENSE_Q8_PREPARED_FIXTURE_ONLY 1
#define PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_DENSE_Q8_UPLOAD_INSPECTION 1
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_q38_dense_q8_prepared_pages_test.cpp"
#include "../PioneerQ38HybridSpanUpload.h"
struct FakeDenseIO {
 const PioneerQ38CanonicalModelPlan& canonical;DensePages& pages;
 unsigned calls=0,fragment=0,page=0,converted=0;uint64_t bytes=0;
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==42 && op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && size==sizeof(tt_q38_span_upload));
  while(fragment<canonical.Count() && canonical.Part(fragment)->kind!=canonical.WeightKind)fragment++;
  assert(fragment<canonical.Count());const auto* p=pages.Part(fragment);assert(p);
  auto& q=*static_cast<tt_q38_span_upload*>(data);const unsigned n=std::min(256U,p->allocated/4096-page);
  assert(q.version==1 && !q.reserved && q.bank==p->bank && q.offset==uint64_t(p->offset)+uint64_t(page)*4096 && q.bytes==n*4096);
  const unsigned expected=p->encoding==Plan::DenseQ8Bfp8 || p->encoding==PioneerQ38HybridModelPlan::Bfp8?0x8b:
   p->encoding==PioneerQ38HybridModelPlan::Bfp4?0x4b:p->encoding==Plan::Parent::HeadBfp8?0x8d:0x5a;
  for(unsigned i=0;i<n;i++)assert(reinterpret_cast<const uint8_t*>(q.data)[i*4096]==expected);
  if(p->encoding==Plan::DenseQ8Bfp8){
   converted+=n;if(page+n==p->allocated/4096 && (p->bytes&4095))
    assert(reinterpret_cast<const uint8_t*>(q.data)[q.bytes-1]==0);
  }
  calls++;bytes+=q.bytes;page+=n;if(page==p->allocated/4096){fragment++;page=0;}
  q.result={1,TT_RUNTIME_PASSED,q.bytes,n,calls,1,1,1,0,0,0,0};return true;
 }
};
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
 FakeDenseIO io{*canonical,pages};PioneerQ38HybridSpanUpload<DensePages,FakeDenseIO,PioneerQ38DenseQ8UploadPlan> upload;
 assert(upload.Run(pages,*canonical,io,42,error) && upload.CompleteHeld());
 assert(upload.UploadedParts()==3551 && upload.UploadedBytes()==io.bytes && io.calls==upload.UploadedSpans() && io.converted>0);
 printf("PASS prepared dense-Q8 pages through %u fake span calls; %u converted pages; NO HARDWARE OR MODEL\n",io.calls,io.converted);
}
