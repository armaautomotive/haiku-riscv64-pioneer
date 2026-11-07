/* SPDX-License-Identifier: MIT */
// Exact QGW8 receipt against synthetic bank proof, ledger, and host plan.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadReceipt.h"
#include "../PioneerQ38HybridHeadModelPlan.h"
#include <cstring>
#include <memory>
#include <vector>

int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--tamper") || !strcmp(argv[1],"--extra")
  || !strcmp(argv[1],"--abort") || !strcmp(argv[1],"--late"))));
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 BanksMock io;auto proof=BankProof(io);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages);
 auto receipt=std::make_unique<PioneerQ38Qgw8UploadReceipt>();int context=0,cookie=0,foreign=0;
 assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
 assert(!receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,42));
 PioneerQ38SpanLease span;assert(!receipt->Seal(span) && !receipt->Sealed());
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 PioneerQ38HybridHeadModelPlan::Proposal host;assert(PioneerQ38HybridHeadModelPlan::Build(*canonical,host));
 unsigned bank=0,length=0,spans=0,part=0,page=0;uint64_t offset=0,total=0;
 if(argc==2 && !strcmp(argv[1],"--abort")){
  assert(receipt->Next(bank,offset,length));assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  receipt->Abort();assert(banks.Quarantined() && !receipt->Sealed() && !receipt->Next(bank,offset,length));
  puts("PASS QGW8 uncertain receipt quarantines synthetic bank owner; no device");return 0;
 }
 while(receipt->Next(bank,offset,length)){
  while(part<canonical->Count() && canonical->Part(part)->kind!=canonical->WeightKind)part++;
  assert(part<canonical->Count());const auto& expected=host.parts[part];
  const unsigned n=std::min(256U,expected.allocated/4096-page);
  assert(bank==expected.bank && offset==uint64_t(expected.offset)+uint64_t(page)*4096 && length==n*4096);
  const auto first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
  assert(!receipt->Before(&context,hashes.data(),ledger.data(),&span,(bank+1)%7,offset,length));
  assert(!receipt->Before(&foreign,hashes.data(),ledger.data(),&span,bank,offset,length));
  assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  assert(!receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  for(unsigned p=0;p<n;p++){
   const uint32_t value=(first+p)*2654435761U;ledger[first+p]=1;hashes[first+p]=value;
   assert(receipt->RecordPage(first+p,value));
  }
  span.saved=span.ready=span.dirty=true;span.nonce=spans+1;
  assert(receipt->After());total+=length;spans++;page+=n;
  if(page==expected.allocated/4096){page=0;part++;}
 }
 assert(spans==22035 && total==UINT64_C(21167624192) && !receipt->Sealed());
 PioneerQ38SpanLease foreignSpan;foreignSpan.saved=true;foreignSpan.closed=true;foreignSpan.nonce=spans;
 assert(!receipt->Seal(foreignSpan));
 span.saved=true;span.closed=true;span.nonce=spans-1;assert(!receipt->Seal(span));
 span.nonce=spans;span.ready=span.dirty=false;
 if(argc==2 && strcmp(argv[1],"--late")){
  if(!strcmp(argv[1],"--tamper"))hashes[0]^=1;
  else {unsigned extra=0;while(extra<ledger.size() && ledger[extra])extra++;assert(extra<ledger.size());ledger[extra]=1;}
  assert(!receipt->Seal(span) && !receipt->Sealed() && banks.Quarantined());
  puts("PASS QGW8 changed/extraneous ledger quarantines before binding; no device");return 0;
 }
 assert(receipt->Seal(span) && receipt->Sealed() && receipt->PageDigest() && receipt->Layout());
 auto bound=std::make_unique<PioneerQ38Qgw8KernelLayout>();
 if(argc==2){hashes[0]^=1;assert(!receipt->DeriveBoundLayout(*bound,&context,&cookie,hashes.data(),ledger.data(),17,42)
  && banks.Quarantined() && !receipt->Sealed());
  puts("PASS QGW8 post-seal ledger mutation quarantines; no device");return 0;}
 assert(!receipt->DeriveBoundLayout(*bound,&context,&foreign,hashes.data(),ledger.data(),17,42));
 assert(receipt->DeriveBoundLayout(*bound,&context,&cookie,hashes.data(),ledger.data(),17,42) && bound->Ready()
  && bound->Count()==4678 && !receipt->DeriveBoundLayout(*bound,&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(!receipt->Seal(span) && !receipt->Next(bank,offset,length));
 assert(receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,42));
 printf("PASS QGW8 receipt: %u host-plan-matched extents, 3551 fragments, %llu bytes; synthetic owner/ledger only, no device\n",
  spans,(unsigned long long)total);
}
