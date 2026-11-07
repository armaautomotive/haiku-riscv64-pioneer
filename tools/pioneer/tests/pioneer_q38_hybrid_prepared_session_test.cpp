/* SPDX-License-Identifier: MIT */
// Connect actual prepared-page dispatch, actual session and actual span packet
// adapter over synthetic cache/model and fake device endpoint; NO DEVICE IO.
#define PIONEER_Q38_HYBRID_PREPARED_UPLOAD_FIXTURE_ONLY 1
#define PIONEER_Q38_HYBRID_JOB_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC 1
#include "pioneer_q38_hybrid_prepared_upload_test.cpp"
#include "../PioneerQ38HybridUploadSession.h"
#include "../PioneerQ38HybridJobBinding.h"
#include "../PioneerQ38TokenStartupPlan.h"

struct PreparedSessionIO {
 UploadIO upload;unsigned calls=0,opens=0,closes=0,begins=0,spanEnds=0,bankEnds=0;bool owned=false;
 std::vector<uint8_t> ledger=std::vector<uint8_t>(PioneerResidentBanks::Pages);
 unsigned recordedPages=0;
 PreparedSessionIO(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38HybridModelPlan::Proposal& p):upload{c,p}{}
 int Open(){assert(!opens++);return 42;}
 int Close(int fd){assert(fd==42 && !closes++);return 0;}
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==42 && opens==1 && !closes);calls++;
  if(op==TT_PCI_PROBE_RELOAD_STATUS){
   auto& q=*static_cast<tt_pioneer_driver_reload*>(data);assert(size==sizeof(q));
   q.driver=55;q.guard_version=1;q.build_tag=UINT64_C(0x2026102100010012);q.generation=20;q.open_count=1;q.active_owner=unsigned(owned);
  }else if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT){
   auto& q=*static_cast<tt_q38_bank_support*>(data);q={45,1,7,4096,4,1,PioneerQ38CanonicalModelPlan::Capacity};
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT){
   auto& q=*static_cast<tt_q38_span_support*>(data);q={55,1,7,4096,1048576,1,1,0};
  }else if(op==TT_PCI_PROBE_Q38_BANK_BEGIN){
   assert(!owned && !begins++);owned=true;auto& q=*static_cast<tt_q38_bank_session*>(data);
   q.result={};q.result.version=1;q.result.state=TT_RESIDENT_SESSION_READY;q.result.active=q.result.host_ready=1;
   q.result.bytes=PioneerResidentBanks::Total;
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD){
   assert(owned && !spanEnds && !bankEnds);
   auto& packet=*static_cast<tt_q38_span_upload*>(data);
   const unsigned first=packet.bank*unsigned(PioneerQ38CanonicalModelPlan::Capacity/4096)+unsigned(packet.offset/4096);
   const unsigned count=packet.bytes/4096;
   assert(first<ledger.size() && count<=ledger.size()-first);
   for(unsigned page=0;page<count;page++)assert(!ledger[first+page]);
   if(!upload.Call(fd,op,data,size))return false;
   for(unsigned page=0;page<count;page++)ledger[first+page]=1;
   recordedPages+=count;return true;
  }else if(op==TT_PCI_PROBE_Q38_SPAN_END){
   assert(owned && !spanEnds++);*static_cast<tt_q38_span_end*>(data)={1,0,1,0};
  }else{
   assert(op==TT_PCI_PROBE_Q38_BANK_END && owned && spanEnds==1 && !bankEnds++);
   auto& q=*static_cast<tt_q38_bank_session*>(data);q.result={};q.result.version=1;
   q.result.state=TT_RESIDENT_SESSION_CLOSED;q.result.pci_restored=q.result.host_released=1;owned=false;
  }
  return true;
 }
};
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
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);assert(p);
  const unsigned tensor=p->kind==canonical->WeightKind?ids.at({p->layer,p->role}):851;
  packed.parts.push_back({p->kind,p->layer,p->role,tensor,p->bank,p->firstRow,p->rows,p->rowBytes,
   canonical->Base+p->offset,p->bytes,p->allocated});
 }
 const auto digest=PioneerModelWeightCache::Hash("authenticated-fixture",21);
 const auto fingerprint=PioneerModelWeightCache::Hash("converter-fingerprint",21);
 FakeBfp8 eight{&source};eight.digest=digest;MockPages pages;std::string error;
 assert(pages.Bind(source,packed,*canonical,eight,digest,fingerprint,"/fixture",[]{return true;},error)
  && pages.VerifyAll(error) && pages.AllVerified());
 FakeFour<FakeSource>::batches=0;FakeFour<FakeSource>::read=0;eight.batches=0;eight.reads=0;
 PreparedSessionIO io(*canonical,proposal);PioneerQ38HybridUploadSession<MockPages,PreparedSessionIO> session(pages,io);
 assert(session.Begin(*canonical,error) && session.WeightsHeld() && io.owned && !io.bankEnds
  && io.calls==22010 && io.upload.calls==22005 && !io.closes);
 // Feed the exact accepted request extents into the same page-presence check
 // used by native startup. A presence-only ledger is NOT format provenance.
 auto relocated=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(relocated->BuildHybridDiagnostic());
 assert(io.recordedPages==UINT64_C(21137039360)/4096);
 assert(PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*relocated,io.ledger.data(),io.ledger.size()));
 assert(PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*canonical,io.ledger.data(),io.ledger.size()));
 // The original plan also passes, although the uploaded bytes are not Q4.
 // Never use that fact to authorize either plan, or to launch a worker.
 const auto* last=relocated->ProjectionStripe(60,4,6);assert(last);
 const unsigned missing=last->bank*unsigned(relocated->Capacity/4096)+last->offset/4096;
 assert(io.ledger[missing]);io.ledger[missing]=0;
 assert(!PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*relocated,io.ledger.data(),io.ledger.size()));
 io.ledger[missing]=1;
 std::array<uint32_t,decltype(session)::RootManifestWords> actualManifest={},expectedManifest={};
 PioneerQ38HybridJobBinding binding;
 assert(binding.Bind(*canonical,UINT64_C(0x1fff000000000000))
  && binding.RootManifest(expectedManifest.data(),expectedManifest.size())
  && session.RootManifest(actualManifest.data(),actualManifest.size()) && actualManifest==expectedManifest);
 unsigned expectedBatches=0,expectedPages=0,eightBatches=0,eightPages=0;
 for(unsigned i=0;i<canonical->Count();i++){
  const auto& p=*canonical->Part(i);
  if(p.kind!=canonical->WeightKind || p.layer>=64 || p.role<2 || p.role>4)continue;
  const unsigned n=proposal.parts[i].allocated/4096;
  if(p.layer>=48 && p.layer<=60){eightPages+=n;eightBatches+=(n+15)/16;}
  else{expectedPages+=n;expectedBatches+=(n+15)/16;}
 }
 assert(FakeFour<FakeSource>::read==expectedPages && FakeFour<FakeSource>::batches==expectedBatches
  && expectedBatches<expectedPages/10 && eight.reads==eightPages && eight.batches==eightBatches && eightBatches<eightPages/10);
 assert(session.Finish(error) && session.Complete() && !session.FailedState()
  && io.calls==22012 && io.upload.calls==22005 && io.closes==1 && !io.owned
  && session.Upload().UploadedBytes()==UINT64_C(21137039360));
 printf("PASS actual prepared-page/owner-session/span composition: BFP4 %u/%u and BFP8 %u/%u synthetic pages/batched reads; BOTH hybrid and original presence-only startup checks accept this ledger, so format binding is required; no device or inference\n",expectedPages,expectedBatches,eightPages,eightBatches);
}
