/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"
#include "../PioneerQ38SmallRecurrentUploadPlan.h"
#include "../PioneerQ38SmallRecurrentUploadReceiptInspection.h"

struct FakePrepared {
 using Plan=PioneerQ38HybridSmallRecurrentBfp8Plan;
 Plan::Proposal proposal;bool live=true;unsigned calls=0,failAt=0;
 bool Ready()const{return live;}bool AllVerified()const{return live;}
 const PioneerQ38HybridModelPlan::Part* Part(unsigned i)const{return live && i<proposal.parts.size()?&proposal.parts[i]:nullptr;}
 bool OutputSafe(const void* out,size_t n)const{
  const uintptr_t x=reinterpret_cast<uintptr_t>(out),self=reinterpret_cast<uintptr_t>(this),limit=~uintptr_t(0);
  return live && out && n && x<=limit-n && !(x<self+sizeof(*this) && self<x+n);
 }
 bool ReadPages(unsigned fragment,unsigned page,unsigned count,uint8_t* out,unsigned capacity,std::string&){
  if(!live || fragment>=proposal.parts.size() || !count || count>16 || capacity<count*4096
   || page>=proposal.parts[fragment].allocated/4096 || count>proposal.parts[fragment].allocated/4096-page || ++calls==failAt)return false;
  for(unsigned i=0;i<count;i++)std::memset(out+i*4096,unsigned((fragment+page+i)%251),4096);
  return true;
 }
};
int main(){
 using Plan=PioneerQ38SmallRecurrentUploadPlan<FakePrepared>;using Receipt=PioneerQ38SmallRecurrentUploadReceiptInspection;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 FakePrepared prepared;assert(FakePrepared::Plan::Build(*canonical,prepared.proposal));std::string error;
 Plan upload;assert(upload.Bind(*canonical,prepared,error) && !upload.Bind(*canonical,prepared,error));
 BanksMock io;auto proof=BankProof(io);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0),buffer(256*4096,0xa5);
 std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);auto receipt=std::make_unique<Receipt>();
 int context=0,cookie=0;assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
 PioneerQ38SpanLease span;unsigned fragment=0,bank=0,length=0,receiptBank=0,receiptLength=0,spans=0;uint64_t offset=0,receiptOffset=0;
 assert(!upload.Next(fragment,fragment,offset,length));
 assert(!upload.Stage(reinterpret_cast<uint8_t*>(&prepared),sizeof(prepared),fragment,bank,offset,length,error) && upload.Ready());
 while(upload.Next(fragment,bank,offset,length)){
  assert(receipt->Next(receiptBank,receiptOffset,receiptLength));
  assert(bank==receiptBank && offset==receiptOffset && length==receiptLength);
  const unsigned startPage=unsigned((offset-prepared.proposal.parts[fragment].offset)/4096),count=length/4096;
  const unsigned oldFragment=fragment;assert(upload.Stage(buffer.data(),buffer.size(),fragment,bank,offset,length,error));
  assert(fragment==oldFragment && bank==receiptBank && offset==receiptOffset && length==receiptLength);
  for(unsigned i=0;i<count;i++)assert(buffer[i*4096]==uint8_t((fragment+startPage+i)%251));
  assert(!upload.Next(fragment,bank,offset,length));
  assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,receiptBank,receiptOffset,receiptLength));
  const unsigned first=unsigned(uint64_t(receiptBank)*(PioneerResidentBanks::Capacity/4096)+receiptOffset/4096);
  for(unsigned i=0;i<count;i++){const uint32_t h=(first+i)*2654435761U;ledger[first+i]=1;hashes[first+i]=h;assert(receipt->RecordPage(first+i,h));}
  span.saved=span.ready=span.dirty=true;span.nonce=++spans;assert(receipt->After() && upload.Acknowledge(true));
 }
 span.closed=true;span.ready=span.dirty=false;
 assert(spans==21475 && upload.Finished() && receipt->Seal(span));
 Plan failed;assert(failed.Bind(*canonical,prepared,error));prepared.failAt=prepared.calls+2;
 std::fill(buffer.begin(),buffer.end(),0xa5);const auto before=buffer;fragment=99;bank=99;offset=99;length=99;
 assert(!failed.Stage(buffer.data(),buffer.size(),fragment,bank,offset,length,error) && buffer==before
  && fragment==99 && bank==99 && offset==99 && length==99 && !failed.Ready());
 prepared.failAt=0;assert(!failed.Ready());
 Plan uncertain;assert(uncertain.Bind(*canonical,prepared,error));assert(uncertain.Stage(buffer.data(),buffer.size(),fragment,bank,offset,length,error));
 assert(!uncertain.Acknowledge(false) && !uncertain.Ready());
 puts("PASS all relocated staged spans match receipt physical order; atomic failure and uncertain-transfer refusal; SYNTHETIC NO DEVICE/SPEED");
}
