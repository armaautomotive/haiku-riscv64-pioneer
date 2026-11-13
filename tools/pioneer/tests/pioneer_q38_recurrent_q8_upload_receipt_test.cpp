/* SPDX-License-Identifier: MIT */
// Synthetic ownership and ledger only. No real upload or device.
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "../PioneerQ38RecurrentQ8MatrixWorkspace.h"
#include "../PioneerQ38RecurrentQ8WorkspaceTransferInspection.h"
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQ38RecurrentQ8UploadReceiptInspection.h"
#include "../PioneerQ38HybridRecurrentQ8MatrixPlan.h"
#include <cstring>
#include <memory>
#include <vector>
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--hole") || !strcmp(argv[1],"--abort") || !strcmp(argv[1],"--bad-record"))));
 BanksMock io;auto proof=BankProof(io);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 auto receipt=std::make_unique<PioneerQ38RecurrentQ8UploadReceiptInspection>();int context=0,cookie=0,foreign=0;
 assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
 PioneerQ38SpanLease span;assert(!receipt->Seal(span));
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 PioneerQ38HybridRecurrentQ8MatrixPlan::Proposal host;assert(PioneerQ38HybridRecurrentQ8MatrixPlan::Build(*canonical,host));
 unsigned bank=0,length=0,spans=0,part=0,page=0,hole=UINT32_MAX;uint64_t offset=0,total=0;
 if(argc==2 && !strcmp(argv[1],"--abort")){
  assert(receipt->Next(bank,offset,length) && receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  receipt->Abort();assert(banks.Quarantined() && !receipt->Sealed() && !receipt->Next(bank,offset,length));
  puts("PASS uncertain recurrent receipt quarantines fake owner");return 0;
 }
 if(argc==2 && !strcmp(argv[1],"--bad-record")){
  assert(receipt->Next(bank,offset,length) && receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  const unsigned first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
  assert(!receipt->RecordPage(first+1,0) && banks.Quarantined() && !receipt->Sealed());
  puts("PASS out-of-order page quarantines fake owner");return 0;
 }
 while(receipt->Next(bank,offset,length)){
  while(part<canonical->Count() && canonical->Part(part)->kind!=canonical->WeightKind)part++;
  assert(part<canonical->Count());const auto& expected=host.parts[part];
  const unsigned n=std::min(256U,expected.allocated/4096-page);
  assert(bank==expected.bank && offset==uint64_t(expected.offset)+uint64_t(page)*4096 && length==n*4096);
  const unsigned first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
  assert(!receipt->Before(&foreign,hashes.data(),ledger.data(),&span,bank,offset,length));
  assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  for(unsigned p=0;p<n;p++){
   const uint32_t value=(first+p)*2654435761U;ledger[first+p]=1;hashes[first+p]=value;
   assert(receipt->RecordPage(first+p,value));
  }
  span.saved=span.ready=span.dirty=true;span.nonce=spans+1;
  assert(receipt->After());total+=length;spans++;page+=n;
  if(page==expected.allocated/4096){
   if(expected.encoding==PioneerQ38HybridRecurrentQ8MatrixPlan::RecurrentQ8Bfp8 && hole==UINT32_MAX)
    hole=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+(expected.offset+expected.allocated)/4096);
   page=0;part++;
  }
 }
 assert(spans==21475 && total==UINT64_C(20748193792) && hole!=UINT32_MAX && !ledger[hole]);
 span.saved=true;span.closed=true;span.nonce=spans;span.ready=span.dirty=false;
 if(argc==2 && !strcmp(argv[1],"--hole")){
  ledger[hole]=1;assert(!receipt->Seal(span) && !receipt->Sealed() && banks.Quarantined());
  puts("PASS unexpected gap page quarantines fake owner");return 0;
 }
 assert(receipt->Seal(span) && receipt->Sealed() && receipt->PageDigest());
 assert(receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(!receipt->Matches(&foreign,&cookie,hashes.data(),ledger.data(),17,42));
 assert(!receipt->Matches(&context,&foreign,hashes.data(),ledger.data(),17,42));
 assert(!receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,43));
 assert(receipt->Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42));
 using W=PioneerQ38RecurrentQ8MatrixWorkspace;
 std::array<uint32_t,W::Words> actual={},expected={};actual.fill(0xa5a5a5a5);
 const auto untouched=actual;
 assert(!receipt->Workspace(actual.data(),actual.size(),&context,&foreign,hashes.data(),ledger.data(),17,42) && actual==untouched);
 assert(!receipt->Workspace(actual.data(),actual.size()-1,&context,&cookie,hashes.data(),ledger.data(),17,42) && actual==untouched);
 assert(!receipt->Workspace(hashes.data(),W::Words,&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(!receipt->Workspace(reinterpret_cast<uint32_t*>(&context),W::Words,&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(W::Build(*canonical,host,expected.data(),expected.size()));
 assert(receipt->Workspace(actual.data(),actual.size(),&context,&cookie,hashes.data(),ledger.data(),17,42) && actual==expected);
 using Transfer=PioneerQ38RecurrentQ8WorkspaceTransferInspection;
 Transfer wrongBank;assert(!wrongBank.Bind(*receipt,&context,&cookie,hashes.data(),ledger.data(),17,42,7,0x04000000));
 Transfer overflow;assert(!overflow.Bind(*receipt,&context,&cookie,hashes.data(),ledger.data(),17,42,2,0xffffffe0));
 Transfer wrongOwner;assert(!wrongOwner.Bind(*receipt,&context,&foreign,hashes.data(),ledger.data(),17,42,2,0x04000000));
 Transfer transfer;assert(transfer.Bind(*receipt,&context,&cookie,hashes.data(),ledger.data(),17,42,2,0x04000000));
 assert(!transfer.Bind(*receipt,&context,&cookie,hashes.data(),ledger.data(),17,42,2,0x04000000));
 std::vector<uint8_t> packed(W::Bytes);PioneerQ38TokenRuntimePlan::Chunk desc={};
 for(unsigned i=0;i<Transfer::Chunks;i++){
  assert(transfer.Describe(i,desc) && desc.bank==2 && desc.address==0x04000000+W::Offset+desc.offset);
  assert(!transfer.Read(i,packed.data()+desc.offset,desc.bytes-1));
  assert(!transfer.Read(i,reinterpret_cast<uint8_t*>(hashes.data()),desc.bytes));
  assert(transfer.Read(i,packed.data()+desc.offset,desc.bytes));
 }
 assert(!transfer.Describe(Transfer::Chunks,desc));
 for(unsigned i=0;i<W::Bytes;i++)assert(packed[i]==uint8_t(expected[i/4]>>((i&3)*8)));
 ledger[hole]=1;assert(!receipt->Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(banks.Quarantined() && !receipt->Sealed());
 assert(!transfer.Describe(0,desc));
 puts("PASS synthetic recurrent span ledger, identity, seal and mutation quarantine; NO DEVICE");
}
