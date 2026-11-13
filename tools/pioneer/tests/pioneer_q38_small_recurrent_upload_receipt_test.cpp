/* SPDX-License-Identifier: MIT */
// Synthetic receipt lifecycle only: never touches a card.
#define PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"
#include "../PioneerQ38SmallRecurrentUploadReceiptInspection.h"

int main(){
 BanksMock io;auto proof=BankProof(io);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 using Receipt=PioneerQ38SmallRecurrentUploadReceiptInspection;auto receipt=std::make_unique<Receipt>();
 int context=0,cookie=0,foreign=0;assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
 PioneerQ38SpanLease span;assert(!receipt->Seal(span));unsigned bank=0,length=0,spans=0,last=0;uint64_t offset=0,pages=0;
 while(receipt->Next(bank,offset,length)){
  const unsigned first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096),count=length/4096;
  assert(!spans || first>=last);last=first+count;
  assert(!receipt->Before(&foreign,hashes.data(),ledger.data(),&span,bank,offset,length));
  assert(!receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset+4096,length));
  assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  for(unsigned i=0;i<count;i++){
   const uint32_t hash=(first+i)*2654435761U;ledger[first+i]=1;hashes[first+i]=hash;
   assert(receipt->RecordPage(first+i,hash));
  }
  span.saved=span.ready=span.dirty=true;span.nonce=++spans;assert(receipt->After());pages+=count;
 }
 assert(spans==21475 && pages==5067109);span.closed=true;span.ready=span.dirty=false;
 assert(receipt->Seal(span) && receipt->Sealed() && receipt->Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(!receipt->Revalidate(&foreign,&cookie,hashes.data(),ledger.data(),17,42));
 unsigned hole=0;while(hole<ledger.size() && ledger[hole])hole++;assert(hole<ledger.size());
 ledger[hole]=1;assert(!receipt->Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42) && banks.Quarantined());
 assert(!receipt->Sealed());
 BanksMock io2;auto proof2=BankProof(io2);PioneerResidentBanks banks2;assert(banks2.Bind(proof2));
 std::fill(ledger.begin(),ledger.end(),0);std::fill(hashes.begin(),hashes.end(),0);
 auto aborted=std::make_unique<Receipt>();assert(aborted->Begin(&context,&cookie,hashes.data(),ledger.data(),17,43,banks2,proof2,ledger.size()));
 PioneerQ38SpanLease second;assert(aborted->Next(bank,offset,length));
 assert(aborted->Before(&context,hashes.data(),ledger.data(),&second,bank,offset,length));aborted->Abort();
 assert(banks2.Quarantined() && !aborted->Next(bank,offset,length) && !aborted->Sealed());
 puts("PASS physical-order 5067109-page synthetic receipt; mutation and uncertain transfer quarantine; NO DEVICE/UPLOAD/SPEED");
}
