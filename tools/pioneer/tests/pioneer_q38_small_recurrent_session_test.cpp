/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SMALL_RECURRENT_SPAN_UPLOAD_FIXTURE_ONLY 1
#define PIONEER_Q38_SMALL_RECURRENT_SESSION_INSPECTION 1
#define PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION 1
#include "pioneer_q38_small_recurrent_span_upload_test.cpp"
#include "../PioneerQ38SmallRecurrentUploadSession.h"

struct SessionIO {
 UploadIO& transfer;PioneerResidentBanks& banks;tt_dram_samples_result& proof;void* cookie;
 unsigned mode=0,opens=0,closes=0,bankEnds=0,spanEnds=0,controls=0;bool owned=false;
 int Open(){opens++;return 7;}int Close(int fd){assert(fd==7);closes++;return 0;}
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==7);
  if(op==TT_PCI_PROBE_RELOAD_STATUS){auto& p=*static_cast<tt_pioneer_driver_reload*>(data);assert(size==sizeof(p) && p.version==1);
   p.driver=55;p.guard_version=1;p.build_tag=mode==1?1:UINT64_C(0x51544b4e4f445256);p.generation=9;p.open_count=1;p.active_owner=owned?1:0;return true;}
  if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT){auto& p=*static_cast<tt_q38_bank_support*>(data);assert(size==sizeof(p));
   p.driver=45;p.version=1;p.banks=7;p.page_bytes=4096;p.max_pages=4;p.storage_only=1;p.bytes_per_bank=PioneerResidentBanks::Capacity;return true;}
  if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT){auto& p=*static_cast<tt_q38_span_support*>(data);assert(size==sizeof(p));
   p.driver=55;p.version=1;p.banks=7;p.page_bytes=4096;p.max_bytes=TT_Q38_SPAN_PUBLIC_BYTES;p.before_cohort=p.explicit_end=1;return true;}
  if(op==TT_PCI_PROBE_Q38_BANK_BEGIN){auto& p=*static_cast<tt_q38_bank_session*>(data);assert(size==sizeof(p) && p.version==1);
   owned=true;p.result.version=1;p.result.state=TT_RESIDENT_SESSION_READY;p.result.active=p.result.host_ready=1;p.result.bytes=PioneerResidentBanks::Total;return true;}
  if(op==TT_PCI_PROBE_Q38_SMALL_RECURRENT_UPLOAD_CONTROL){namespace Q=PioneerQ38SmallRecurrentUpload;
   auto& p=*static_cast<Q::Packet*>(data);assert(size==sizeof(p) && Q::Request(p) && owned);controls++;
   if(mode==2)return false;
   if(p.operation==Q::Begin){assert(transfer.receipt.Begin(transfer.context,cookie,transfer.hashes,transfer.ledger,17,42,banks,proof,PioneerResidentBanks::Pages));}
   else {assert(transfer.receipt.Seal(transfer.span));p.sealed=1;p.spans=transfer.calls;}
   p.accepted=1;if(mode==4 && p.operation==Q::Seal)return false;return true;}
  if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD){if(mode==3)transfer.mode=1;return transfer.Call(fd,op,data,size);}
  if(op==TT_PCI_PROBE_Q38_SPAN_END){auto& p=*static_cast<tt_q38_span_end*>(data);assert(size==sizeof(p) && p.version==1);spanEnds++;
   transfer.span.closed=true;transfer.span.ready=transfer.span.dirty=false;p.closed=1;return true;}
  if(op==TT_PCI_PROBE_Q38_BANK_END){auto& p=*static_cast<tt_q38_bank_session*>(data);assert(size==sizeof(p) && owned);bankEnds++;owned=false;
   p.result.version=1;p.result.state=TT_RESIDENT_SESSION_CLOSED;p.result.pci_restored=p.result.host_released=1;return true;}
  assert(false);return false;
 }
};
#ifndef PIONEER_Q38_SMALL_RECURRENT_SESSION_FIXTURE_ONLY
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 FakePrepared prepared;assert(FakePrepared::Plan::Build(*canonical,prepared.proposal));
 std::vector<unsigned> order;for(unsigned i=0;i<canonical->Count();i++)if(canonical->Part(i)->kind==PioneerQ38CanonicalModelPlan::WeightKind)order.push_back(i);
 std::sort(order.begin(),order.end(),[&](unsigned a,unsigned b){const auto& x=prepared.proposal.parts[a];const auto& y=prepared.proposal.parts[b];
  return std::make_pair(x.bank,x.offset)<std::make_pair(y.bank,y.offset);});
 for(unsigned mode=0;mode<5;mode++){
  BanksMock backend;auto proof=BankProof(backend);PioneerResidentBanks banks;assert(banks.Bind(proof));
  std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
  auto receipt=std::make_unique<PioneerQ38SmallRecurrentUploadReceiptInspection>();int context=0,cookie=0;
  PioneerQ38SpanLease span;UploadIO transfer{*receipt,span,prepared,order,&context,hashes.data(),ledger.data(),0,0};
  SessionIO io{transfer,banks,proof,&cookie,mode};std::string error;
  {PioneerQ38SmallRecurrentUploadSession<FakePrepared,SessionIO> session(prepared,io);
   const bool ok=session.Begin(*canonical,error);
   if(mode==0){assert(ok && error.empty() && session.WeightsHeld() && io.closes==0 && io.controls==2 && io.spanEnds==1
     && io.bankEnds==0 && transfer.calls==21475);assert(session.Finish(error) && io.bankEnds==1 && io.closes==1 && !session.WeightsHeld());}
   else {assert(!ok && session.FailedState() && !session.WeightsHeld() && io.closes==1 && !io.bankEnds);
    assert(io.spanEnds==(mode==4?1U:0U));if(mode==4)assert(io.controls==2 && transfer.calls==21475);}
   assert(!session.Begin(*canonical,error) && io.opens==1);
  }
  assert(io.closes==1);
 }
 puts("PASS fake-identity prepared upload session, sealed receipt and held owner; uncertain operations close once without bank END; SIMULATED ONLY");
}
#endif
