/* SPDX-License-Identifier: MIT */
// Actual loader, span serializer and kernel receipt; synthetic model/device.
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT 1
#define PIONEER_Q38_HYBRID_SPAN_FIXTURE_ONLY 1
#include "pioneer_q38_hybrid_span_upload_test.cpp"
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQ38HybridUploadSession.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridUploadReceipt.h"

struct TrackedIO {
 FakeIO spanIO;BanksMock bankIO;tt_dram_samples_result proof;PioneerResidentBanks banks;
 std::vector<uint8_t> ledger=std::vector<uint8_t>(PioneerResidentBanks::Pages);
 std::vector<uint32_t> hashes=std::vector<uint32_t>(PioneerResidentBanks::Pages);
 PioneerQ38HybridUploadReceipt receipt;PioneerQ38SpanLease lease;
 int context=0,cookie=0;unsigned calls=0,opens=0,closes=0,begin=0,seal=0,bankEnd=0,spanEnd=0;
 bool owned=false,retained=false,failBegin=false,failSeal=false,badSeal=false;unsigned badBeginField=0;uint64_t wrongTag=0;
 TrackedIO(const Canonical& c,FakePages& p):spanIO{c,p},proof(BankProof(bankIO)){assert(banks.Bind(proof));}
 int Open(){assert(!opens++);return 42;}
 int Close(int fd){assert(fd==42 && !closes++);if(owned && begin)retained=true;return 0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==42 && opens==1 && !closes);calls++;
  if(op==TT_PCI_PROBE_RELOAD_STATUS){
   auto& q=*static_cast<tt_pioneer_driver_reload*>(data);assert(bytes==sizeof(q));
   q.driver=55;q.guard_version=1;q.build_tag=wrongTag?wrongTag:
#ifdef PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT
#ifdef PIONEER_Q38_HYBRID_FIXED_126
    UINT64_C(0x2026110700010126);
#else
    UINT64_C(0x2026110700010125);
#endif
#else
    UINT64_C(0x51544b4e4f445256);
#endif
   q.generation=20;q.open_count=1;q.active_owner=unsigned(owned);
  }else if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT){
   auto& q=*static_cast<tt_q38_bank_support*>(data);q={45,1,7,4096,4,1,Canonical::Capacity};
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT){
   auto& q=*static_cast<tt_q38_span_support*>(data);q={55,1,7,4096,1048576,1,1,0};
  }else if(op==TT_PCI_PROBE_Q38_BANK_BEGIN){
   assert(!owned);owned=true;auto& q=*static_cast<tt_q38_bank_session*>(data);q.result={};
   q.result.version=1;q.result.state=TT_RESIDENT_SESSION_READY;q.result.active=q.result.host_ready=1;q.result.bytes=PioneerResidentBanks::Total;
  }else if(op==TT_PCI_PROBE_Q38_HYBRID_UPLOAD_CONTROL){
   namespace H=PioneerQ38HybridUpload;auto& q=*static_cast<H::Packet*>(data);assert(bytes==sizeof(q) && H::Request(q) && owned);
   if(q.operation==H::Begin){assert(!begin++ && !spanEnd);
    assert(receipt.Begin(&context,&cookie,hashes.data(),ledger.data(),17,20,banks,proof,ledger.size()));
    if(failBegin){receipt.Invalidate();return false;}q.accepted=1;
    if(badBeginField){uint32_t* fields[]={&q.magic,&q.version,&q.operation,&q.reserved,&q.accepted,&q.sealed,&q.spans,&q.recovery_required};
     assert(badBeginField<=8);*fields[badBeginField-1]^=1;}
   }else {assert(q.operation==H::Seal && begin==1 && spanEnd==1 && !seal++);
    assert(receipt.Seal(lease) && receipt.Matches(&context,&cookie,hashes.data(),ledger.data(),17,20));
    if(failSeal){receipt.Invalidate();return false;}q.accepted=q.sealed=1;q.spans=receipt.Spans();if(badSeal)q.spans--;}
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD){
   auto& q=*static_cast<tt_q38_span_upload*>(data);assert(owned && begin==1 && !spanEnd);
   assert(receipt.Before(&context,hashes.data(),ledger.data(),&lease,q.bank,q.offset,q.bytes));
   assert(spanIO.Call(fd,op,data,bytes));
   const unsigned first=q.bank*unsigned(Canonical::Capacity/4096)+unsigned(q.offset/4096);
   for(unsigned p=0;p<q.bytes/4096;p++){
    const unsigned index=first+p;const uint32_t hash=index*2654435761U;
    ledger[index]=1;hashes[index]=hash;assert(receipt.RecordPage(index,hash));
   }
   lease.saved=lease.ready=lease.dirty=true;lease.nonce++;assert(receipt.After());
  }else if(op==TT_PCI_PROBE_Q38_SPAN_END){
   assert(owned && begin==1 && !spanEnd++);lease.ready=lease.dirty=false;lease.closed=true;
   *static_cast<tt_q38_span_end*>(data)={1,0,1,0};
  }else{
   assert(op==TT_PCI_PROBE_Q38_BANK_END && owned && seal==1 && !bankEnd++);
   auto& q=*static_cast<tt_q38_bank_session*>(data);q.result={};q.result.version=1;
   q.result.state=TT_RESIDENT_SESSION_CLOSED;q.result.pci_restored=q.result.host_released=1;owned=false;
  }
  return true;
 }
};
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--seal-failure") || !strcmp(argv[1],"--bad-seal"))));
 auto canonical=std::make_unique<Canonical>();assert(canonical->Build());std::string error;
 using Session=PioneerQ38HybridUploadSession<FakePages,TrackedIO,true>;
 if(argc==1)for(uint64_t tag:{UINT64_C(0x2026102100010012),
#ifdef PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT
  UINT64_C(0x51544b4e4f445256),
#ifdef PIONEER_Q38_HYBRID_FIXED_126
  UINT64_C(0x2026110700010125)
#else
  UINT64_C(0x2026110700010126)
#endif
#else
  UINT64_C(0x2026110700010125),UINT64_C(0x2026110700010126)
#endif
 }){
  FakePages p(*canonical);TrackedIO io(*canonical,p);Session s(p,io);io.wrongTag=tag;
  assert(!s.Run(*canonical,error) && s.FailedState() && io.calls==1 && io.closes==1 && !io.owned);
 }
 if(argc==1){
  FakePages p(*canonical);TrackedIO io(*canonical,p);Session s(p,io);io.failBegin=true;
  assert(!s.Run(*canonical,error) && s.FailedState() && io.calls==5 && io.closes==1 && io.owned
   && io.retained && io.banks.Quarantined() && !io.spanEnd && !io.bankEnd);
 }
 if(argc==1)for(unsigned field=1;field<=8;field++){
  FakePages p(*canonical);TrackedIO io(*canonical,p);Session s(p,io);io.badBeginField=field;
  assert(!s.Run(*canonical,error) && s.FailedState() && io.calls==5 && io.closes==1 && io.owned
   && io.retained && !io.spanEnd && !io.bankEnd);
 }
 FakePages pages(*canonical);TrackedIO io(*canonical,pages);Session session(pages,io);
 io.failSeal=argc==2 && !strcmp(argv[1],"--seal-failure");io.badSeal=argc==2 && !strcmp(argv[1],"--bad-seal");
 if(argc==2){
  assert(!session.Begin(*canonical,error) && session.FailedState() && !session.WeightsHeld() && io.calls==22012
   && io.owned && io.retained && (!io.failSeal || io.banks.Quarantined()) && io.closes==1 && io.spanEnd==1 && io.seal==1 && !io.bankEnd);
  puts("PASS uncertain tracked SEAL is close-only after full synthetic upload; NO DEVICE");return 0;
 }
 assert(session.Begin(*canonical,error) && session.WeightsHeld() && io.calls==22012 && io.owned && !io.closes);
 assert(io.receipt.Sealed() && io.receipt.Matches(&io.context,&io.cookie,io.hashes.data(),io.ledger.data(),17,20));
 assert(session.Finish(error) && session.Complete() && io.calls==22014 && !io.owned && io.closes==1 && io.bankEnd==1);
 puts("PASS tracked loader + kernel receipt: full synthetic upload, same FD, BEGIN/SPAN_END/SEAL/BANK_END; NO DEVICE OR INFERENCE");
 return 0;
}
