/* SPDX-License-Identifier: MIT */
// Real session and span packet sequence over simulated endpoints ONLY.
#define PIONEER_Q38_HYBRID_SPAN_FIXTURE_ONLY 1
#include "pioneer_q38_hybrid_span_upload_test.cpp"
#include "../PioneerQ38HybridUploadSession.h"

struct SessionIO {
 FakeIO span;unsigned calls=0,opens=0,closes=0,begins=0,spanEnds=0,bankEnds=0,statuses=0;
 unsigned failCall=0,badCall=0,revokeCall=0;bool stop=false,owned=false,failOpen=false,failClose=false;
 SessionIO(const Canonical& c,FakePages& p):span{c,p}{}
 int Open(){assert(!opens++ && !closes);return failOpen?-1:42;}
 int Close(int fd){assert(fd==42 && !closes++);return failClose?-1:0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==42 && opens==1 && !closes && !stop);calls++;
  if(calls==failCall){stop=true;return false;}
  if(op==TT_PCI_PROBE_RELOAD_STATUS){
   statuses++;auto& q=*static_cast<tt_pioneer_driver_reload*>(data);assert(bytes==sizeof(q) && q.version==1);
   q.driver=55;q.guard_version=1;q.build_tag=UINT64_C(0x2026102100010012);q.generation=20;
   q.active_owner=unsigned(owned);q.open_count=1;if(calls==badCall)q.generation++;
   if(calls==badCall && statuses==1)q.build_tag^=1;
  }else if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT){
   auto& q=*static_cast<tt_q38_bank_support*>(data);assert(bytes==sizeof(q));
   q={45,1,7,4096,4,1,Canonical::Capacity};if(calls==badCall)q.banks=6;
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT){
   auto& q=*static_cast<tt_q38_span_support*>(data);assert(bytes==sizeof(q));
   q={55,1,7,4096,1048576,1,1,0};if(calls==badCall)q.reserved=1;
  }else if(op==TT_PCI_PROBE_Q38_BANK_BEGIN){
   assert(!owned && !begins++);owned=true;auto& q=*static_cast<tt_q38_bank_session*>(data);assert(bytes==sizeof(q));
   q.result={};q.result.version=1;q.result.state=TT_RESIDENT_SESSION_READY;q.result.active=q.result.host_ready=1;
   q.result.bytes=PioneerResidentBanks::Total;if(calls==badCall)q.result.active=0;
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD){
   assert(owned && !spanEnds && !bankEnds);if(calls==badCall)span.badField=5;
   assert(span.Call(fd,op,data,bytes));
  }else if(op==TT_PCI_PROBE_Q38_SPAN_END){
   assert(owned && !spanEnds++ && !bankEnds);auto& q=*static_cast<tt_q38_span_end*>(data);assert(bytes==sizeof(q));
   q={1,0,1,0};if(calls==badCall)q.recovery_required=1;
  }else{
   assert(op==TT_PCI_PROBE_Q38_BANK_END && owned && spanEnds==1 && !bankEnds++);
   auto& q=*static_cast<tt_q38_bank_session*>(data);assert(bytes==sizeof(q));q.result={};q.result.version=1;
   q.result.state=TT_RESIDENT_SESSION_CLOSED;q.result.pci_restored=q.result.host_released=1;owned=false;
   if(calls==badCall)q.result.host_released=0;
  }
  if(calls==badCall)stop=true;
  if(calls==revokeCall)span.pages.live=false;
  return true;
 }
};
int main(){
 auto canonical=std::make_unique<Canonical>();assert(canonical->Build());std::string error;
 using Session=PioneerQ38HybridUploadSession<FakePages,SessionIO>;
 {
  FakePages pages(*canonical);SessionIO io(*canonical,pages);Session session(pages,io);pages.live=false;
  assert(!session.Run(*canonical,error) && session.FailedState() && !io.opens && !io.calls);
  pages.live=true;assert(!session.Run(*canonical,error) && !io.opens);
 }
 {
  FakePages pages(*canonical);SessionIO io(*canonical,pages);Session session(pages,io);io.failOpen=true;
  assert(!session.Run(*canonical,error) && session.FailedState() && io.opens==1 && !io.calls && !io.closes);
 }
 for(unsigned call:{4U,5U}){
  FakePages pages(*canonical);SessionIO io(*canonical,pages);Session session(pages,io);io.revokeCall=call;
  assert(!session.Run(*canonical,error) && session.FailedState() && io.calls==call && io.closes==1 && io.owned);
 }
 // Every early failure or bad reply is close-only, with no following device
 // operation. Four late controls also cover SPAN_END/BANK_END/final identity.
 for(unsigned call:{1U,2U,3U,4U,5U,22010U,22011U,22012U})for(unsigned bad:{0U,1U}){
  FakePages pages(*canonical);SessionIO io(*canonical,pages);Session session(pages,io);
  if(bad)io.badCall=call;else io.failCall=call;
  assert(!session.Run(*canonical,error) && session.FailedState() && !session.Complete()
   && io.calls==call && io.opens==1 && io.closes==1);
  assert(!session.Run(*canonical,error) && io.calls==call && io.closes==1);
 }
 {
  FakePages pages(*canonical);SessionIO io(*canonical,pages);Session session(pages,io);unsigned progress=0;
  assert(!session.Finish(error) && !io.opens && !io.calls);
  assert(session.Begin(*canonical,error,[&](uint64_t done,uint64_t total){assert(done<=total);progress++;}));
  // SPAN_END restored the upload machinery, but BANK_END has not run: the
  // exact uploaded weights and the exclusive bank session are still held.
  assert(session.WeightsHeld() && !session.Complete() && !session.FailedState()
   && io.calls==22010 && io.begins==1 && io.spanEnds==1 && !io.bankEnds && io.owned && !io.closes);
  std::array<uint32_t,Session::RootManifestWords> manifest;manifest.fill(0xdeadbeef);
  assert(!session.RootManifest(manifest.data(),manifest.size()-1) && manifest[0]==0xdeadbeef);
  assert(session.RootManifest(manifest.data(),manifest.size()));
  const auto first=manifest[0];pages.proposal.parts[0].offset+=4096;
  assert(!session.RootManifest(manifest.data(),manifest.size()) && manifest[0]==first);
  pages.proposal.parts[0].offset-=4096;
  unsigned converted=0;
  while(converted<pages.proposal.parts.size() && pages.proposal.parts[converted].encoding!=PioneerQ38HybridModelPlan::Bfp4)converted++;
  assert(converted<pages.proposal.parts.size());
  pages.proposal.parts[converted].encoding=PioneerQ38HybridModelPlan::Original;
  assert(!session.RootManifest(manifest.data(),manifest.size()) && manifest[0]==first);
  pages.proposal.parts[converted].encoding=PioneerQ38HybridModelPlan::Bfp4;
  const auto prior=pages.proposal.parts[0];
  assert(!session.RootManifest(reinterpret_cast<uint32_t*>(pages.proposal.parts.data()),manifest.size())
   && pages.proposal.parts[0].offset==prior.offset && pages.proposal.parts[0].original==prior.original);
  assert(!session.Begin(*canonical,error) && session.WeightsHeld() && io.calls==22010);
  assert(session.Finish(error));
  assert(session.Complete() && !session.FailedState() && io.calls==22012 && io.begins==1
   && io.spanEnds==1 && io.bankEnds==1 && io.statuses==2 && !io.owned && io.closes==1
   && session.Upload().UploadedParts()==3551 && session.Upload().UploadedSpans()==22005 && progress==22005);
  assert(!session.WeightsHeld() && !session.RootManifest(manifest.data(),manifest.size())
   && !session.Finish(error) && io.calls==22012 && io.closes==1);
 }
 {
  FakePages pages(*canonical);SessionIO io(*canonical,pages);Session session(pages,io);
  assert(session.Begin(*canonical,error) && session.WeightsHeld());
  pages.live=false;assert(!session.WeightsHeld());
  std::array<uint32_t,Session::RootManifestWords> manifest;manifest.fill(0xdeadbeef);
  assert(!session.RootManifest(manifest.data(),manifest.size()) && manifest[0]==0xdeadbeef);
  // Revoked source must not be treated as a clean BANK_END: close only.
  assert(!session.Finish(error) && session.FailedState() && io.calls==22010
   && io.owned && !io.bankEnds && io.closes==1);
 }
 {
  FakePages pages(*canonical);SessionIO io(*canonical,pages);Session session(pages,io);io.failClose=true;
  assert(!session.Run(*canonical,error) && session.FailedState() && !session.Complete()
   && io.calls==22012 && io.closes==1 && !io.owned);
 }
 puts("PASS isolated upload session: actual owner/span/END packet sequence with fake transport, full 21137039360 synthetic bytes, failure boundaries/source revocation/close-only; NO DEVICE OR INFERENCE");
}
