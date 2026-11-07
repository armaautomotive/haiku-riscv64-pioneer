/* SPDX-License-Identifier: MIT */
// Real QGW8 page router, span builder, session and kernel receipt joined by
// a simulated owner/transport. No real device, model bytes, or inference.
#define PIONEER_Q38_QGW8_SPAN_BRIDGE_FIXTURE_ONLY
#include "pioneer_q38_hybrid_head_span_bridge_test.cpp"
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_QGW8_SPAN_INSPECTION 1
#define PIONEER_Q38_QGW8_CONTROL_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_QGW8_USER_SESSION_INSPECTION 1
#include "../PioneerQ38Qgw8UploadSessionInspection.h"
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadReceipt.h"

struct SimOwner {
 Fixture& fixture;FakeTransport spans;BanksMock bankIO;tt_dram_samples_result proof;PioneerResidentBanks banks;
 std::vector<uint8_t> ledger=std::vector<uint8_t>(PioneerResidentBanks::Pages);
 std::vector<uint32_t> hashes=std::vector<uint32_t>(PioneerResidentBanks::Pages);
 std::unique_ptr<PioneerQ38Qgw8UploadReceipt> receipt=std::make_unique<PioneerQ38Qgw8UploadReceipt>();
 PioneerQ38SpanLease lease;int context=0,cookie=0;
 unsigned calls=0,opens=0,closes=0,begins=0,seals=0,spanEnd=0,bankEnd=0;
 bool owned=false,retained=false,badBegin=false,failSpan=false,badSeal=false;uint64_t tag=0;
 explicit SimOwner(Fixture& f):fixture(f),spans{f},proof(BankProof(bankIO)){assert(banks.Bind(proof));}
 int Open(){assert(!opens++);return 42;}
 int Close(int fd){assert(fd==42 && !closes++);if(owned){retained=true;receipt->Invalidate();}return 0;}
 bool Control(int fd,PioneerQ38Qgw8Upload::Packet& q){
  namespace Q=PioneerQ38Qgw8Upload;assert(fd==42 && owned && Q::Request(q));calls++;
  if(q.operation==Q::Begin){
   assert(!begins++ && !spanEnd);
   assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,20,banks,proof,ledger.size()));
   q.accepted=1;if(badBegin)q.magic^=1;return true;
  }
  assert(q.operation==Q::Seal && begins==1 && spanEnd==1 && !seals++);
  assert(receipt->Seal(lease) && receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,20));
  q.accepted=q.sealed=1;q.spans=receipt->Spans();if(badSeal)q.spans--;return true;
 }
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==42 && opens==1 && !closes);calls++;
  if(op==TT_PCI_PROBE_RELOAD_STATUS){
   auto& q=*static_cast<tt_pioneer_driver_reload*>(data);assert(size==sizeof(q));
   q.driver=55;q.guard_version=1;q.build_tag=tag?tag:UINT64_C(0x385747514f4e4452);
   q.generation=20;q.open_count=1;q.active_owner=unsigned(owned);
  }else if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT){
   *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,Canonical::Capacity};
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT){
   *static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};
  }else if(op==TT_PCI_PROBE_Q38_BANK_BEGIN){
   assert(!owned);owned=true;auto& q=*static_cast<tt_q38_bank_session*>(data);q.result={};
   q.result.version=1;q.result.state=TT_RESIDENT_SESSION_READY;q.result.active=q.result.host_ready=1;q.result.bytes=PioneerResidentBanks::Total;
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD){
   auto& q=*static_cast<tt_q38_span_upload*>(data);assert(owned && begins==1 && !spanEnd);
   assert(receipt->Before(&context,hashes.data(),ledger.data(),&lease,q.bank,q.offset,q.bytes));
   if(failSpan){receipt->Abort();return false;}
   assert(spans.Call(fd,op,data,size));
   const unsigned first=q.bank*unsigned(Canonical::Capacity/4096)+unsigned(q.offset/4096);
   for(unsigned p=0;p<q.bytes/4096;p++){
    const unsigned index=first+p;const uint32_t hash=index*2654435761U;
    ledger[index]=1;hashes[index]=hash;assert(receipt->RecordPage(index,hash));
   }
   lease.saved=lease.ready=lease.dirty=true;lease.nonce++;assert(receipt->After());
  }else if(op==TT_PCI_PROBE_Q38_SPAN_END){
   assert(owned && begins==1 && !spanEnd++);lease.ready=lease.dirty=false;lease.closed=true;
   *static_cast<tt_q38_span_end*>(data)={1,0,1,0};
  }else{
   assert(op==TT_PCI_PROBE_Q38_BANK_END && owned && seals==1 && !bankEnd++);
   auto& q=*static_cast<tt_q38_bank_session*>(data);q.result={};q.result.version=1;
   q.result.state=TT_RESIDENT_SESSION_CLOSED;q.result.pci_restored=q.result.host_released=1;owned=false;
  }
  return true;
 }
};
using Session=PioneerQ38Qgw8UploadSessionInspection<HeadPages,SimOwner>;
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && !strcmp(argv[1],"--bad-seal")));
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 if(argc==1){
  Fixture f;auto io=std::make_unique<SimOwner>(f);Session session(f.pages,*io);std::string error;f.source.live=false;
  assert(!session.Begin(*f.canonical,error) && session.FailedState() && !io->opens && !io->calls && !io->closes);
 }
 if(argc==1){
  Fixture f;auto io=std::make_unique<SimOwner>(f);Session session(f.pages,*io);std::string error;
  io->tag=UINT64_C(0x2026102100010012); // actual old driver must never be admitted
  assert(!session.Begin(*f.canonical,error) && session.FailedState() && io->calls==1 && io->closes==1 && !io->owned);
 }
 if(argc==1){
  Fixture f;auto io=std::make_unique<SimOwner>(f);Session session(f.pages,*io);std::string error;io->badBegin=true;
  assert(!session.Begin(*f.canonical,error) && session.FailedState() && io->closes==1 && io->owned && io->retained
   && io->banks.Quarantined() && !io->spanEnd && !io->bankEnd);
 }
 if(argc==1){
  Fixture f;auto io=std::make_unique<SimOwner>(f);Session session(f.pages,*io);std::string error;io->failSpan=true;
  assert(!session.Begin(*f.canonical,error) && session.FailedState() && io->closes==1 && io->owned && io->retained
   && io->banks.Quarantined() && !io->spanEnd && !io->bankEnd);
 }
 Fixture f;auto io=std::make_unique<SimOwner>(f);Session session(f.pages,*io);std::string error;io->badSeal=argc==2;
 if(io->badSeal){
  assert(!session.Begin(*f.canonical,error) && session.FailedState() && io->owned && io->retained
   && io->closes==1 && io->spanEnd==1 && io->seals==1 && !io->bankEnd && io->banks.Quarantined());
  puts("PASS QGW8 bad synthetic SEAL reply retains/quarantines without END/retry; no device");return 0;
 }
 assert(session.Begin(*f.canonical,error) && session.WeightsHeld() && io->owned && !io->closes);
 assert(io->receipt->Sealed() && io->receipt->Matches(&io->context,&io->cookie,io->hashes.data(),io->ledger.data(),17,20));
 std::array<uint32_t,Session::RootManifestWords> actual={},expected={};
 actual.fill(0x12345678);const auto unchanged=actual;
 assert(!session.RootManifest(actual.data(),actual.size()-1) && actual==unchanged);
 assert(!session.RootManifest(reinterpret_cast<uint32_t*>(&session),Session::RootManifestWords));
 assert(session.RootManifest(actual.data(),actual.size())
  && PioneerQ38HybridHeadModelPlan::Manifest(*f.canonical,f.proposal,expected.data(),expected.size()) && actual==expected);
 assert(session.Finish(error) && session.Complete() && !io->owned && io->closes==1 && io->bankEnd==1);
 assert(!session.WeightsHeld() && !session.RootManifest(actual.data(),actual.size()));
 printf("PASS QGW8 actual page router/session/receipt: %u synthetic spans, %llu bytes, matching ROOT manifest, clean END; no model/device/TPS\n",
  session.Upload().UploadedSpans(),(unsigned long long)session.Upload().UploadedBytes());
}
