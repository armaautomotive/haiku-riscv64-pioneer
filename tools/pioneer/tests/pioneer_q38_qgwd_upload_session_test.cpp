/* SPDX-License-Identifier: MIT */
// Actual QGWD page router, span adapter and upload-session lifecycle, joined
// to the gapped-ledger receipt by a synthetic owner. No hardware or model.
#define PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION 1
#define PIONEER_Q38_QGW8_SPAN_INSPECTION 1
#define PIONEER_Q38_QGW8_CONTROL_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_QGW8_USER_SESSION_INSPECTION 1
#define PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_DENSE_Q8_UPLOAD_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38Qgw8UploadSessionInspection.h"
#define PIONEER_Q38_DENSE_Q8_PREPARED_UPLOAD_FIXTURE_ONLY 1
#include "pioneer_q38_dense_q8_prepared_upload_test.cpp"

struct SessionOwner {
 FakeDenseIO& spans;PioneerQ38DenseQ8UploadReceiptInspection& receipt;PioneerQ38SpanLease& lease;
 PioneerResidentBanks& banks;const tt_dram_samples_result& proof;std::vector<uint8_t>& ledger;
 std::vector<uint32_t>& hashes;int context=0,cookie=0;bool owned=false;unsigned opens=0,closes=0,seals=0;
 uint64_t tag=UINT64_C(0x445747514f4e4452);
 bool OutputSafe(const void* output,size_t bytes)const{
  const uintptr_t o=reinterpret_cast<uintptr_t>(output);
  if(!output || !bytes || o>UINTPTR_MAX-bytes)return false;
  auto overlap=[&](const void* p,size_t n){const uintptr_t a=reinterpret_cast<uintptr_t>(p);
   return n && (a>UINTPTR_MAX-n || (o<a+n && a<o+bytes));};
  return !overlap(ledger.data(),ledger.size()) && !overlap(hashes.data(),hashes.size()*sizeof(hashes[0]))
   && !overlap(&receipt,sizeof(receipt));
 }
 int Open(){assert(!opens++);return 42;}
 int Close(int fd){assert(fd==42 && !closes++);if(owned)banks.Quarantine();return 0;}
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==42 && opens==1 && !closes);
  if(op==TT_PCI_PROBE_RELOAD_STATUS){
   auto& q=*static_cast<tt_pioneer_driver_reload*>(data);assert(size==sizeof(q));
   q.driver=55;q.guard_version=1;q.build_tag=tag;
   q.generation=42;q.open_count=1;q.active_owner=unsigned(owned);return true;
  }
  if(op==TT_PCI_PROBE_Q38_QGWD_UPLOAD_CONTROL){
   namespace Q=PioneerQ38QgwdUpload;auto& q=*static_cast<Q::Packet*>(data);assert(size==sizeof(q) && owned && Q::Request(q));
   if(q.operation==Q::Begin){assert(receipt.Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
    spans.context=&context;q.accepted=1;return true;}
   assert(q.operation==Q::Seal && !seals++ && receipt.Seal(lease)
    && receipt.Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42));
   q.accepted=q.sealed=1;q.spans=receipt.Spans();return true;
  }
  if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT){*static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,PioneerQ38CanonicalModelPlan::Capacity};return true;}
  if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT){*static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};return true;}
  if(op==TT_PCI_PROBE_Q38_BANK_BEGIN){assert(!owned);owned=true;auto& q=*static_cast<tt_q38_bank_session*>(data);q.result={};
   q.result.version=1;q.result.state=TT_RESIDENT_SESSION_READY;q.result.active=q.result.host_ready=1;q.result.bytes=PioneerResidentBanks::Total;return true;}
  if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD)return spans.Call(fd,op,data,size);
  if(op==TT_PCI_PROBE_Q38_SPAN_END){lease.ready=lease.dirty=false;lease.closed=true;*static_cast<tt_q38_span_end*>(data)={1,0,1,0};return true;}
  assert(op==TT_PCI_PROBE_Q38_BANK_END && owned && seals==1);auto& q=*static_cast<tt_q38_bank_session*>(data);q.result={};
  q.result.version=1;q.result.state=TT_RESIDENT_SESSION_CLOSED;q.result.pci_restored=q.result.host_released=1;owned=false;return true;
 }
};

int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());FakeSource source;FakePlan packed;packed.source=&source;
 std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){PioneerQ38Tensor t;if(!PioneerQ38Expected(layer,role,t))break;
   ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(t);}}
 for(unsigned i=0;i<canonical->Count();i++){const auto* p=canonical->Part(i);assert(p);
  const unsigned tensor=p->kind==canonical->WeightKind?ids.at({p->layer,p->role}):851;
  packed.parts.push_back({p->kind,p->layer,p->role,tensor,p->bank,p->firstRow,p->rows,p->rowBytes,canonical->Base+p->offset,p->bytes,p->allocated});}
 const auto digest=PioneerModelWeightCache::Hash("authenticated-fixture",21);
 const auto four=PioneerModelWeightCache::Hash("four-fingerprint",16),head=PioneerModelWeightCache::Hash("head-fingerprint",16);
 FakeBfp8 eight{&source};eight.digest=digest;std::string error;HeadPages parent;
 assert(parent.Bind(source,packed,*canonical,eight,digest,four,head,"/private/tmp/four","/private/tmp/head",[]{return true;},error));
 assert(parent.VerifyAll(error));DensePages pages;assert(pages.Bind(source,packed,*canonical,eight,parent,digest,error) && pages.VerifyAll(error));
 BanksMock mock;auto proof=BankProof(mock);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 auto receipt=std::make_unique<PioneerQ38DenseQ8UploadReceiptInspection>();PioneerQ38SpanLease lease;int initial=0;
 FakeDenseIO spans{*canonical,pages,*receipt,lease,&initial,ledger,hashes};
 {SessionOwner wrong{spans,*receipt,lease,banks,proof,ledger,hashes};wrong.tag=UINT64_C(0x2026102100010012);
  PioneerQ38Qgw8UploadSessionInspection<DensePages,SessionOwner> denied(pages,wrong);
  assert(!denied.Begin(*canonical,error) && denied.FailedState() && wrong.opens==1 && wrong.closes==1 && !wrong.owned);}
 {PioneerQ38QgwdUpload::Packet dense;PioneerQ38Qgw8Upload::Packet old;dense.operation=old.operation=1;
  assert(PioneerQ38QgwdUpload::Request(dense) && PioneerQ38Qgw8Upload::Request(old));
  dense.magic=old.magic;assert(!PioneerQ38QgwdUpload::Request(dense));}
 SessionOwner io{spans,*receipt,lease,banks,proof,ledger,hashes};
 using Session=PioneerQ38Qgw8UploadSessionInspection<DensePages,SessionOwner>;Session session(pages,io);
 assert(session.Begin(*canonical,error) && session.WeightsHeld() && io.owned && receipt->Sealed());
 std::array<uint32_t,Session::RootManifestWords> actual={},expected={};PioneerQ38HybridHeadModelPlan::Proposal logical;
 assert(PioneerQ38HybridHeadModelPlan::Build(*canonical,logical)
  && PioneerQ38HybridHeadModelPlan::Manifest(*canonical,logical,expected.data(),expected.size())
  && session.RootManifest(actual.data(),actual.size()) && actual==expected);
 namespace H=PioneerQ38HybridHeadLayout;std::array<uint32_t,64> boot={};std::array<uint32_t,H::Words> headMetadata={};
 boot[0]=H::C::Magic;boot[1]=1;boot[2]=64;boot[3]=129;boot[4]=boot[16]=0x14000;boot[5]=119;boot[6]=8192;boot[7]=64;
 for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
 for(unsigned i=40;i<46;i++)boot[i]=H::C::BootWord(i);
 assert(H::C::BootShape(boot.data()) && session.HeadMetadata(boot.data(),boot.size(),0,headMetadata.data(),headMetadata.size()));
 assert(session.Finish(error) && session.Complete() && !io.owned && io.closes==1 && !banks.Quarantined());
 puts("PASS QGWD prepared pages through actual synthetic upload session, gapped receipt and logical ROOT manifest; NO DEVICE OR MODEL");
}
