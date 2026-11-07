/* SPDX-License-Identifier: MIT */
// Real QGW8 page router, span builder, session and kernel receipt joined by
// a simulated owner/transport. No real device, model bytes, or inference.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_QGW8_SPAN_BRIDGE_FIXTURE_ONLY
#include "pioneer_q38_hybrid_head_span_bridge_test.cpp"
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
 bool OutputSafe(const void* output,size_t bytes)const{
  const uintptr_t o=reinterpret_cast<uintptr_t>(output);if(!output || !bytes || o>UINTPTR_MAX-bytes)return false;
  auto overlaps=[&](const void* p,size_t size){
   const uintptr_t a=reinterpret_cast<uintptr_t>(p);return size && (a>UINTPTR_MAX-size || (o<a+size && a<o+bytes));
  };
  return !overlaps(ledger.data(),ledger.size()) && !overlaps(hashes.data(),hashes.size()*sizeof(hashes[0]))
   && !overlaps(receipt.get(),sizeof(*receipt));
 }
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
static std::array<uint32_t,64> RootBoot(){
 namespace H=PioneerQ38HybridHeadLayout;std::array<uint32_t,64> b={};
 b[0]=H::C::Magic;b[1]=1;b[2]=64;b[3]=129;b[4]=b[16]=0x14000;b[5]=119;b[6]=8192;b[7]=64;
 for(unsigned i=0;i<4;i++)b[8+i*2]=i+1;
 for(unsigned i=40;i<46;i++)b[i]=H::C::BootWord(i);
 assert(H::C::BootShape(b.data()));return b;
}
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--bad-seal") || !strcmp(argv[1],"--emit-head-fixture"))));
 const bool emit=argc==2 && !strcmp(argv[1],"--emit-head-fixture");
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
 Fixture f;auto io=std::make_unique<SimOwner>(f);Session session(f.pages,*io);std::string error;
 io->badSeal=argc==2 && !strcmp(argv[1],"--bad-seal");
 if(io->badSeal){
  assert(!session.Begin(*f.canonical,error) && session.FailedState() && io->owned && io->retained
   && io->closes==1 && io->spanEnd==1 && io->seals==1 && !io->bankEnd && io->banks.Quarantined());
  puts("PASS QGW8 bad synthetic SEAL reply retains/quarantines without END/retry; no device");return 0;
 }
 const auto boot=RootBoot();std::array<uint32_t,128> head={};head.fill(0xa5a5a5a5);const auto untouched=head;
 assert(!session.HeadMetadata(boot.data(),boot.size(),0,head.data(),head.size()) && head==untouched);
 assert(session.Begin(*f.canonical,error) && session.WeightsHeld() && io->owned && !io->closes);
 assert(io->receipt->Sealed() && io->receipt->Matches(&io->context,&io->cookie,io->hashes.data(),io->ledger.data(),17,20));
 std::array<uint32_t,Session::RootManifestWords> actual={},expected={};
 actual.fill(0x12345678);const auto unchanged=actual;
 assert(!session.RootManifest(actual.data(),actual.size()-1) && actual==unchanged);
 assert(!session.RootManifest(reinterpret_cast<uint32_t*>(&session),Session::RootManifestWords));
 assert(!session.RootManifest(reinterpret_cast<uint32_t*>(io.get()),Session::RootManifestWords));
 assert(!session.RootManifest(io->hashes.data(),Session::RootManifestWords));
 assert(!session.RootManifest(reinterpret_cast<uint32_t*>(f.original.parts.data()),Session::RootManifestWords));
 assert(!session.RootManifest(reinterpret_cast<uint32_t*>(f.source.tensors.data()),Session::RootManifestWords));
 assert(session.RootManifest(actual.data(),actual.size())
  && PioneerQ38HybridHeadModelPlan::Manifest(*f.canonical,f.proposal,expected.data(),expected.size()) && actual==expected);
 assert(!session.HeadMetadata(boot.data(),boot.size()-1,0,head.data(),head.size()) && head==untouched);
 assert(!session.HeadMetadata(boot.data(),boot.size(),7,head.data(),head.size()) && head==untouched);
 assert(!session.HeadMetadata(boot.data(),boot.size(),0,head.data(),head.size()-1) && head==untouched);
 auto badBoot=boot;badBoot[0]^=1;
 assert(!session.HeadMetadata(badBoot.data(),badBoot.size(),0,head.data(),head.size()) && head==untouched);
 auto aliasedBoot=boot;const auto savedBoot=aliasedBoot;
 assert(!session.HeadMetadata(aliasedBoot.data(),aliasedBoot.size(),0,aliasedBoot.data(),128) && aliasedBoot==savedBoot);
 assert(!session.HeadMetadata(boot.data(),boot.size(),0,reinterpret_cast<uint32_t*>(&session),128));
 assert(!session.HeadMetadata(boot.data(),boot.size(),0,reinterpret_cast<uint32_t*>(io.get()),128));
 assert(!session.HeadMetadata(boot.data(),boot.size(),0,io->hashes.data(),128));
 assert(!session.HeadMetadata(boot.data(),boot.size(),0,reinterpret_cast<uint32_t*>(f.original.parts.data()),128));
 assert(!session.HeadMetadata(boot.data(),boot.size(),0,reinterpret_cast<uint32_t*>(f.source.tensors.data()),128));
 for(unsigned bank=0;bank<7;bank++){
  assert(session.HeadMetadata(boot.data(),boot.size(),bank,head.data(),head.size()));
  assert(PioneerQ38HybridHeadLayout::Shape(head.data(),boot.data(),bank,head[4]));
  for(unsigned i=0;i<f.canonical->Count();i++){
   const auto* p=f.canonical->Part(i);const auto& q=f.proposal.parts[i];assert(p);
   if(p->kind==Canonical::WorkspaceKind && p->bank==bank)assert(head[4]==Canonical::Base+q.offset);
   if(p->kind==Canonical::WeightKind && p->layer==64 && p->role==2)
    assert(head[6]==q.bank && head[7]==Canonical::Base+q.offset);
   if(p->kind==Canonical::WeightKind && p->layer==64 && p->role==1){
    const auto* stripe=head.data()+PioneerQ38HybridHeadLayout::Stripe+p->bank*PioneerQ38HybridHeadLayout::StripeWords;
    assert(stripe[0]==p->bank && stripe[1]==Canonical::Base+q.offset && stripe[2]==p->firstRow
     && stripe[3]==p->rows && stripe[4]==5440 && stripe[5]==q.bytes && stripe[6]==q.allocated && stripe[7]==139);
   }
  }
  for(unsigned b=0;b<7;b++){
   const auto* stripe=head.data()+PioneerQ38HybridHeadLayout::Stripe+b*PioneerQ38HybridHeadLayout::StripeWords;
   assert(stripe[0]==b && stripe[4]==5440 && stripe[7]==139);
  }
 }
 // Derive all jobs from this held session's descriptor, not an injected
 // standalone head descriptor; every vocabulary row must appear once.
 assert(session.HeadMetadata(boot.data(),boot.size(),0,head.data(),head.size()));
 std::vector<uint8_t> coverage(248320);unsigned covered=0;
 for(unsigned ordinal=0;ordinal<PioneerQ38HybridHeadLayout::Jobs;ordinal++){
  namespace W=PioneerQ38HybridHeadWorkerLayout;
  const unsigned worker=ordinal%119+1;std::array<uint32_t,64> workerBoot={},job={};
  workerBoot[0]=W::Magic;workerBoot[1]=1;workerBoot[2]=64;workerBoot[3]=worker;
  workerBoot[4]=W::Peer(worker);workerBoot[5]=workerBoot[16]=0x14000;workerBoot[6]=8192;workerBoot[7]=64;
  for(unsigned i=0;i<8;i++)workerBoot[8+i]=boot[8+i];
  for(unsigned i=40;i<=50;i++)workerBoot[i]=W::BootWord(i);
  assert(PioneerQ38HybridHeadLayout::Job(head.data(),ordinal,3,17,workerBoot.data(),job.data())
   && W::JobShape(job.data(),workerBoot.data(),3,0));
  const unsigned first=job[32],rows=job[20]*32;assert(first<coverage.size() && rows<=coverage.size()-first);
  for(unsigned row=first;row<first+rows;row++){assert(!coverage[row]);coverage[row]=1;covered++;}
 }
 assert(covered==248320);for(uint8_t seen:coverage)assert(seen==1);
 if(emit){
  static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"simulation fixture uses little endian words");
  namespace W=PioneerQ38HybridHeadWorkerLayout;std::array<uint32_t,64> workerBoot={},job={};
  workerBoot[0]=W::Magic;workerBoot[1]=1;workerBoot[2]=64;workerBoot[3]=1;workerBoot[4]=W::Peer(1);
  workerBoot[5]=workerBoot[16]=0x14000;workerBoot[6]=8192;workerBoot[7]=64;
  for(unsigned i=0;i<8;i++)workerBoot[8+i]=boot[8+i];
  for(unsigned i=40;i<=50;i++)workerBoot[i]=W::BootWord(i);
  assert(PioneerQ38HybridHeadLayout::Job(head.data(),0,0,0,workerBoot.data(),job.data()));
  std::array<uint32_t,256> fixture={};
  std::copy(boot.begin(),boot.end(),fixture.begin());std::copy(head.begin(),head.end(),fixture.begin()+64);
  std::copy(workerBoot.begin(),workerBoot.end(),fixture.begin()+192);
  auto print=[](const char* label,const void* bytes,size_t count){
   printf("%s ",label);const auto* p=static_cast<const uint8_t*>(bytes);
   for(size_t i=0;i<count;i++)printf("%02x",unsigned(p[i]));putchar('\n');
  };
  print("QGW8_HELD_HEAD_FIXTURE",fixture.data(),sizeof(fixture));
  print("QGW8_HELD_HEAD_JOB",job.data(),sizeof(job));
 }
 const auto savedHead=head;f.original.parts[0].bytes++;
 assert(!session.HeadMetadata(boot.data(),boot.size(),0,head.data(),head.size()) && head==savedHead);
 f.original.parts[0].bytes--;
 assert(session.Finish(error) && session.Complete() && !io->owned && io->closes==1 && io->bankEnd==1);
 assert(!session.WeightsHeld() && !session.RootManifest(actual.data(),actual.size())
  && !session.HeadMetadata(boot.data(),boot.size(),0,head.data(),head.size()));
 printf("PASS QGW8 actual page router/session/receipt: %u synthetic spans, %llu bytes, matching FFN/head metadata and 973 jobs, clean END; no model/device/TPS\n",
  session.Upload().UploadedSpans(),(unsigned long long)session.Upload().UploadedBytes());
}
