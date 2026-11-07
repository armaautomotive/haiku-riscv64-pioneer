/* SPDX-License-Identifier: MIT */
// Actual prepared-page adapter and actual span-wire packets, with simulated
// model/cache bytes and a fake transport. No card or learned model access.
#define PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC 1
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_PREPARED_PAGES_FIXTURE_ONLY 1
#include "pioneer_q38_hybrid_prepared_pages_test.cpp"
#include "../PioneerQ38HybridSpanUpload.h"

struct UploadIO {
 const PioneerQ38CanonicalModelPlan& canonical;
 const PioneerQ38HybridModelPlan::Proposal& proposal;
 unsigned fragment=0,page=0,calls=0;uint64_t bytes=0;
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==42 && op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && size==sizeof(tt_q38_span_upload));
  while(fragment<canonical.Count() && canonical.Part(fragment)->kind!=canonical.WeightKind)fragment++;
  assert(fragment<canonical.Count());
  const auto& original=*canonical.Part(fragment);const auto& part=proposal.parts[fragment];
  auto& packet=*static_cast<tt_q38_span_upload*>(data);
  const unsigned count=std::min(256U,part.allocated/4096-page);
  assert(packet.version==1 && !packet.reserved && packet.bank==part.bank
   && packet.offset==uint64_t(part.offset)+uint64_t(page)*4096 && packet.bytes==count*4096);
  for(unsigned p=0;p<count;p++){
   const auto* raw=reinterpret_cast<const uint8_t*>(packet.data)+p*4096;
   const bool ffn=original.layer<64 && original.role>=2 && original.role<=4;
   const bool eight=ffn && original.layer>=48 && original.layer<=60;
   const uint8_t first=ffn?(eight?0x8b:0x4b):0x5a;
   const uint64_t lastOffset=uint64_t(page+p)*4096+4095;
   assert(raw[0]==first && raw[4095]==(eight && lastOffset>=part.bytes?0:first));
  }
  bytes+=packet.bytes;page+=count;if(page==part.allocated/4096){fragment++;page=0;}
  calls++;packet.result={1,TT_RUNTIME_PASSED,packet.bytes,count,calls,1,1,1,0,0,0,0};return true;
 }
};
#ifndef PIONEER_Q38_HYBRID_PREPARED_UPLOAD_FIXTURE_ONLY
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 PioneerQ38HybridModelPlan::Proposal proposal;
 assert(PioneerQ38HybridModelPlan::Build(*canonical,UINT64_C(0x1fff000000000000),proposal));
 FakeSource source;FakePlan packed;packed.source=&source;std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){
  const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){
   PioneerQ38Tensor tensor;if(!PioneerQ38Expected(layer,role,tensor))break;
   ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(tensor);
  }
 }
 assert(source.tensors.size()==851);
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);assert(p);
  const unsigned tensor=p->kind==canonical->WeightKind?ids.at({p->layer,p->role}):851;
  packed.parts.push_back({p->kind,p->layer,p->role,tensor,p->bank,p->firstRow,p->rows,p->rowBytes,
   canonical->Base+p->offset,p->bytes,p->allocated});
 }
 const auto digest=PioneerModelWeightCache::Hash("authenticated-fixture",21);
 const auto fingerprint=PioneerModelWeightCache::Hash("converter-fingerprint",21);
 FakeBfp8 eight{&source};eight.digest=digest;MockPages pages;std::string error;
 assert(pages.Bind(source,packed,*canonical,eight,digest,fingerprint,"/fixture",[]{return true;},error));
 assert(pages.VerifyAll(error) && pages.AllVerified());
 UploadIO io{*canonical,proposal};PioneerQ38HybridSpanUpload<MockPages,UploadIO> upload;
 assert(upload.Run(pages,*canonical,io,42,error));
 assert(upload.CompleteHeld() && upload.UploadedParts()==3551 && upload.UploadedSpans()==22005
  && upload.UploadedBytes()==UINT64_C(21137039360) && io.bytes==upload.UploadedBytes()
  && io.calls==upload.UploadedSpans() && pages.AllVerified());
 printf("PASS actual hybrid prepared-page dispatch into %u fake span calls, %llu synthetic bytes; "
  "NO card, model data, inference or performance result\n",io.calls,(unsigned long long)io.bytes);
}
#endif
