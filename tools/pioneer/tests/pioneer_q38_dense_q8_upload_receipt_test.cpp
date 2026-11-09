/* SPDX-License-Identifier: MIT */
// Complete synthetic QGWD receipt lifecycle; no device or actual model.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQ38DenseQ8UploadReceiptInspection.h"
#include "../PioneerQ38HybridDenseQ8MatrixPlan.h"
#include <cstring>
#include <memory>
#include <vector>
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--hole") || !strcmp(argv[1],"--late") || !strcmp(argv[1],"--abort"))));
 BanksMock io;auto proof=BankProof(io);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 auto receipt=std::make_unique<PioneerQ38DenseQ8UploadReceiptInspection>();int context=0,cookie=0,foreign=0;
 assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
 PioneerQ38SpanLease span;assert(!receipt->Seal(span));
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 PioneerQ38HybridDenseQ8MatrixPlan::Proposal host;assert(PioneerQ38HybridDenseQ8MatrixPlan::Build(*canonical,host));
 unsigned bank=0,length=0,spans=0,part=0,page=0,hole=UINT32_MAX;uint64_t offset=0,total=0;
 if(argc==2 && !strcmp(argv[1],"--abort")){
  assert(receipt->Next(bank,offset,length) && receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  receipt->Abort();assert(banks.Quarantined() && !receipt->Sealed() && !receipt->Next(bank,offset,length));
  puts("PASS uncertain dense-Q8 receipt quarantines fake owner");return 0;
 }
 while(receipt->Next(bank,offset,length)){
  while(part<canonical->Count() && canonical->Part(part)->kind!=canonical->WeightKind)part++;
  assert(part<canonical->Count());const auto& expected=host.parts[part];
  const unsigned n=std::min(256U,expected.allocated/4096-page);
  assert(bank==expected.bank && offset==uint64_t(expected.offset)+uint64_t(page)*4096 && length==n*4096);
  const auto first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
  assert(!receipt->Before(&foreign,hashes.data(),ledger.data(),&span,bank,offset,length));
  assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  for(unsigned p=0;p<n;p++){
   const uint32_t value=(first+p)*2654435761U;ledger[first+p]=1;hashes[first+p]=value;
   assert(receipt->RecordPage(first+p,value));
  }
  span.saved=span.ready=span.dirty=true;span.nonce=spans+1;
  assert(receipt->After());total+=length;spans++;page+=n;
  if(page==expected.allocated/4096){
   if(expected.encoding==PioneerQ38HybridDenseQ8MatrixPlan::DenseQ8Bfp8 && hole==UINT32_MAX)
    hole=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+(expected.offset+expected.allocated)/4096);
   page=0;part++;
  }
 }
 assert(spans==21907 && total==UINT64_C(21094223872) && hole!=UINT32_MAX && !ledger[hole]);
 span.saved=true;span.closed=true;span.nonce=spans;span.ready=span.dirty=false;
 if(argc==2 && !strcmp(argv[1],"--hole")){
  ledger[hole]=1;assert(!receipt->Seal(span) && !receipt->Sealed() && banks.Quarantined());
  puts("PASS unexpected gap page quarantines fake owner");return 0;
 }
 assert(receipt->Seal(span) && receipt->Sealed() && receipt->PageDigest());
 assert(!receipt->Revalidate(&context,&foreign,hashes.data(),ledger.data(),17,42));
 assert(receipt->Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42));
 if(argc==2 && !strcmp(argv[1],"--late")){
  hashes[0]^=1;assert(!receipt->Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42) && banks.Quarantined());
  puts("PASS changed sealed dense-Q8 ledger quarantines fake owner");return 0;
 }
 puts("PASS 21907 synthetic dense-Q8 receipt spans, sealed gapped ledger and bound owner identity; NO DEVICE OR MODEL");
}
