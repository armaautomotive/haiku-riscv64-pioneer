/* SPDX-License-Identifier: MIT */
// Actual QGW8 prepared-page router -> actual span packet builder over a
// synthetic source and transport. No OS device, real model or inference.
#define PIONEER_Q38_HYBRID_HEAD_PREPARED_FIXTURE_ONLY
#include "pioneer_q38_hybrid_head_prepared_pages_test.cpp"
#define PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC 1
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38HybridSpanUpload.h"

using Canonical=PioneerQ38CanonicalModelPlan;
struct Fixture {
 std::unique_ptr<Canonical> canonical=std::make_unique<Canonical>();
 FakeSource source;FakePlan original;FakeBfp8 eight{&source};HeadPages pages;
 PioneerQ38HybridHeadModelPlan::Proposal proposal;std::string error;
 Fixture(){
  assert(canonical->Build());original.source=&source;std::map<std::pair<unsigned,unsigned>,unsigned> ids;
  for(unsigned ordinal=0;ordinal<=64;ordinal++){
   const unsigned layer=ordinal?ordinal-1:64;
   for(unsigned role=0;;role++){
    PioneerQ38Tensor t;if(!PioneerQ38Expected(layer,role,t))break;
    ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(t);
   }
  }
  assert(source.tensors.size()==851);
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* c=canonical->Part(i);assert(c);
   const unsigned tensor=c->kind==canonical->WeightKind?ids.at({c->layer,c->role}):851;
   original.parts.push_back({c->kind,c->layer,c->role,tensor,c->bank,c->firstRow,c->rows,c->rowBytes,
    canonical->Base+c->offset,c->bytes,c->allocated});
  }
  const auto digest=PioneerModelWeightCache::Hash("authenticated-fixture",21);
  const auto four=PioneerModelWeightCache::Hash("four-fingerprint",16);
  const auto head=PioneerModelWeightCache::Hash("head-fingerprint",16);
  eight.digest=digest;
  assert(PioneerQ38HybridHeadModelPlan::Build(*canonical,proposal));
  assert(pages.Bind(source,original,*canonical,eight,digest,four,head,"/private/tmp/four","/private/tmp/head",[]{return true;},error));
  assert(pages.VerifyAll(error) && pages.AllVerified());
 }
};
struct FakeTransport {
 Fixture& f;unsigned calls=0,part=0,page=0;uint64_t bytes=0;bool fail=false,badReply=false;
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==42 && op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && size==sizeof(tt_q38_span_upload));
  calls++;if(fail)return false;
  while(part<f.canonical->Count() && f.canonical->Part(part)->kind!=Canonical::WeightKind)part++;
  assert(part<f.canonical->Count());const auto& p=f.proposal.parts[part];
  auto& packet=*static_cast<tt_q38_span_upload*>(data);
  const unsigned count=std::min(256U,p.allocated/4096-page);
  assert(packet.version==1 && !packet.reserved && packet.bank==p.bank
   && packet.offset==uint64_t(p.offset)+uint64_t(page)*4096 && packet.bytes==count*4096);
  const uint8_t expected=p.encoding==PioneerQ38HybridHeadModelPlan::HeadBfp8?0x8d:
   p.encoding==PioneerQ38HybridModelPlan::Bfp8?0x8b:p.encoding==PioneerQ38HybridModelPlan::Bfp4?0x4b:0x5a;
  const auto* raw=reinterpret_cast<const uint8_t*>(packet.data);
  for(unsigned i=0;i<count;i++)assert(raw[i*4096]==expected);
  bytes+=packet.bytes;page+=count;if(page==p.allocated/4096){part++;page=0;}
  packet.result={1,TT_RUNTIME_PASSED,packet.bytes,count,calls,1,1,1,0,0,0,0};
  if(badReply)packet.result.nonce++;
  return true;
 }
};
struct ForgedPages {
 HeadPages& pages;unsigned index;PioneerQ38HybridModelPlan::Part forged;
 bool AllVerified()const{return pages.AllVerified();}
 const PioneerQ38HybridModelPlan::Part* Part(unsigned i)const{return i==index?&forged:pages.Part(i);}
 bool ReadPages(unsigned i,unsigned p,unsigned n,uint8_t* out,unsigned cap,std::string& e){return pages.ReadPages(i,p,n,out,cap,e);}
};
int main(){
 Fixture f;FakeTransport transport{f};std::string error;
 // Legacy selection must reject the new head BEFORE any transport call.
 PioneerQ38HybridSpanUpload<HeadPages,FakeTransport> legacy;
 assert(!legacy.Run(f.pages,*f.canonical,transport,42,error) && !legacy.DeviceTouched() && !transport.calls);
 unsigned head=0;while(head<f.proposal.parts.size() && f.proposal.parts[head].encoding!=PioneerQ38HybridHeadModelPlan::HeadBfp8)head++;
 assert(head<f.proposal.parts.size());ForgedPages forged{f.pages,head,f.proposal.parts[head]};forged.forged.encoding=PioneerQ38HybridModelPlan::Original;
 PioneerQ38HybridSpanUpload<ForgedPages,FakeTransport,PioneerQ38HybridHeadUploadPlan> refused;
 assert(!refused.Run(forged,*f.canonical,transport,42,error) && !refused.DeviceTouched() && !transport.calls);
 FakeTransport uncertain{f};uncertain.fail=true;
 PioneerQ38HybridSpanUpload<HeadPages,FakeTransport,PioneerQ38HybridHeadUploadPlan> stopped;
 assert(!stopped.Run(f.pages,*f.canonical,uncertain,42,error) && stopped.Failed()
  && stopped.DeviceTouched() && !stopped.CompleteHeld() && uncertain.calls==1);
 uncertain.fail=false;assert(!stopped.Run(f.pages,*f.canonical,uncertain,42,error) && uncertain.calls==1);
 FakeTransport corrupt{f};corrupt.badReply=true;
 PioneerQ38HybridSpanUpload<HeadPages,FakeTransport,PioneerQ38HybridHeadUploadPlan> badReceipt;
 assert(!badReceipt.Run(f.pages,*f.canonical,corrupt,42,error) && badReceipt.Failed()
  && badReceipt.DeviceTouched() && !badReceipt.CompleteHeld() && corrupt.calls==1);
 corrupt.badReply=false;assert(!badReceipt.Run(f.pages,*f.canonical,corrupt,42,error) && corrupt.calls==1);
 PioneerQ38HybridSpanUpload<HeadPages,FakeTransport,PioneerQ38HybridHeadUploadPlan> upload;
 uint64_t expected=0;for(unsigned i=0;i<f.canonical->Count();i++)
  if(f.canonical->Part(i)->kind==Canonical::WeightKind)expected+=f.proposal.parts[i].allocated;
 assert(upload.Run(f.pages,*f.canonical,transport,42,error));
 assert(upload.CompleteHeld() && upload.UploadedParts()==3551 && upload.UploadedBytes()==expected
  && expected==UINT64_C(21167624192) && upload.UploadedSpans()==22035
  && upload.PlannedBytes()==expected && transport.bytes==expected && transport.calls==upload.UploadedSpans());
 printf("PASS actual prepared-page router -> span packet builder: %u synthetic parts, %u packets, %llu bytes; NO MODEL, DEVICE, OWNERSHIP OR TPS\n",
  upload.UploadedParts(),upload.UploadedSpans(),(unsigned long long)expected);
}
