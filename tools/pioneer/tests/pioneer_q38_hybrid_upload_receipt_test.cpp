/* SPDX-License-Identifier: MIT */
// Actual kernel receipt and canonical layout; synthetic ledger and bank proof.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridUploadReceipt.h"
#include <memory>
#include <cstring>
#include <vector>

int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--tamper") || !strcmp(argv[1],"--extra")
  || !strcmp(argv[1],"--abort") || !strcmp(argv[1],"--late"))));
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 BanksMock io;auto proof=BankProof(io);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages);
 auto receipt=std::make_unique<PioneerQ38HybridUploadReceipt>();int context=0,cookie=0,foreign=0;
 assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
 assert(!receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,42)); // not sealed yet
 assert(!receipt->Matches(&context,&foreign,hashes.data(),ledger.data(),17,42));
 assert(!receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,43));
 PioneerQ38SpanLease span;assert(!receipt->Seal(span) && !receipt->Sealed());
 unsigned bank=0,length=0;uint64_t offset=0,total=0;unsigned spans=0;
 if(argc==2 && !strcmp(argv[1],"--abort")){
  assert(receipt->Next(bank,offset,length));assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  receipt->Abort();assert(banks.Quarantined() && !receipt->Sealed() && !receipt->Next(bank,offset,length));
  puts("PASS uncertain upload receipt quarantines bank ownership; NO DEVICE");return 0;
 }
 while(receipt->Next(bank,offset,length)){
  const auto first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
  assert(!receipt->Before(&context,hashes.data(),ledger.data(),&span,(bank+1)%7,offset,length));
  assert(!receipt->Before(&foreign,hashes.data(),ledger.data(),&span,bank,offset,length));
  assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  assert(!receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  for(unsigned p=0;p<length/4096;p++){
   const uint32_t value=(first+p)*2654435761U;ledger[first+p]=1;hashes[first+p]=value;
   assert(receipt->RecordPage(first+p,value));
  }
  span.saved=span.ready=span.dirty=true;span.nonce=spans+1;
  assert(receipt->After());total+=length;spans++;
 }
 assert(spans==22005 && total==UINT64_C(21137039360) && !receipt->Sealed());
 PioneerQ38SpanLease foreignSpan;foreignSpan.saved=true;foreignSpan.closed=true;foreignSpan.nonce=spans;
 assert(!receipt->Seal(foreignSpan));
 span.saved=true;span.closed=true;span.nonce=spans-1;assert(!receipt->Seal(span));
 span.nonce=spans;span.ready=span.dirty=false;
 if(argc==2 && strcmp(argv[1],"--late")){
  if(!strcmp(argv[1],"--tamper"))hashes[0]^=1;
  else {unsigned extra=0;while(extra<ledger.size() && ledger[extra])extra++;assert(extra<ledger.size());ledger[extra]=1;}
  assert(!receipt->Seal(span) && !receipt->Sealed() && banks.Quarantined());
  puts("PASS changed or extraneous upload ledger quarantines before binding; NO DEVICE");return 0;
 }
 assert(receipt->Seal(span) && receipt->Sealed() && receipt->PageDigest());
 assert(receipt->Layout() && receipt->Layout()->HybridFormatLayout() && !receipt->Layout()->OriginalFormatLayout());
 auto nativeLayout=std::make_unique<PioneerQ38CanonicalModelPlan>();
 if(argc==2){hashes[0]^=1;assert(!receipt->DeriveBoundLayout(*nativeLayout,&context,&cookie,hashes.data(),ledger.data(),17,42)
  && banks.Quarantined() && !receipt->Sealed());
  puts("PASS post-seal ledger change quarantines before binding; NO DEVICE");return 0;}
 assert(!receipt->DeriveBoundLayout(*nativeLayout,&context,&foreign,hashes.data(),ledger.data(),17,42));
 assert(receipt->DeriveBoundLayout(*nativeLayout,&context,&cookie,hashes.data(),ledger.data(),17,42) && nativeLayout->HybridFormatLayout()
  && nativeLayout->Count()==receipt->Layout()->Count()
  && !receipt->DeriveBoundLayout(*nativeLayout,&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(!receipt->Seal(span) && !receipt->Next(bank,offset,length));
 assert(receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,42));
 printf("PASS hybrid receipt: %u ordered extents, 3551 fragments, %llu bytes; exact owner/FD/team/generation; SYNTHETIC LEDGER ONLY\n",
  spans,(unsigned long long)total);
}
