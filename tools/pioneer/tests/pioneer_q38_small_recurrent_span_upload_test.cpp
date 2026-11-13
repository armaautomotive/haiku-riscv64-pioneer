/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SMALL_RECURRENT_UPLOAD_PLAN_FIXTURE_ONLY 1
#define B_DEVICE_OP_CODES_END 10000
#include "pioneer_q38_small_recurrent_upload_plan_test.cpp"
#include "../PioneerQ38SmallRecurrentSpanUpload.h"

struct UploadIO {
 PioneerQ38SmallRecurrentUploadReceiptInspection& receipt;PioneerQ38SpanLease& span;
 FakePrepared& prepared;const std::vector<unsigned>& order;
 void* context;uint32_t* hashes;uint8_t* ledger;unsigned calls=0,mode=0;
 bool Call(int fd,unsigned operation,void* data,size_t size){
  assert(fd==7 && operation==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && size==sizeof(tt_q38_span_upload));
  auto& p=*static_cast<tt_q38_span_upload*>(data);unsigned bank=0,length=0;uint64_t offset=0;
  assert(receipt.Next(bank,offset,length) && p.bank==bank && p.offset==offset && p.bytes==length);
  assert(PioneerQ38SpanPublicShape(p.version,p.bank,p.offset,p.bytes,p.reserved));calls++;
  const auto key=std::make_pair(bank,offset);
  auto it=std::upper_bound(order.begin(),order.end(),key,[&](const auto& k,unsigned i){
   const auto& part=prepared.proposal.parts[i];return k<std::make_pair(part.bank,uint64_t(part.offset));
  });
  assert(it!=order.begin());const unsigned fragment=*--it;const auto& part=prepared.proposal.parts[fragment];
  assert(part.bank==bank && offset>=part.offset && offset-part.offset+length<=part.allocated);
  const unsigned firstPage=unsigned((offset-part.offset)/4096);const auto* bytes=reinterpret_cast<const uint8_t*>(p.data);
  for(unsigned i=0;i<length/4096;i++)assert(bytes[i*4096]==uint8_t((fragment+firstPage+i)%251));
  if(mode==1)return false;
  if(mode==2){p.result.version=1;p.result.state=TT_RUNTIME_PASSED;p.result.nonce=999;return true;}
  assert(receipt.Before(context,hashes,ledger,&span,bank,offset,length));
  const unsigned first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
  for(unsigned i=0;i<length/4096;i++){
   const uint32_t h=(first+i)*2654435761U;ledger[first+i]=1;hashes[first+i]=h;assert(receipt.RecordPage(first+i,h));
  }
  span.saved=span.ready=span.dirty=true;span.nonce=calls;assert(receipt.After());
  p.result.version=1;p.result.state=TT_RUNTIME_PASSED;p.result.bytes=length;p.result.pages=length/4096;
  p.result.nonce=calls;p.result.started=p.result.completed=p.result.held=1;
  if(mode==3)p.result.recovery_required=1;
  if(mode==4)p.offset+=4096;
  return true;
 }
};
#ifndef PIONEER_Q38_SMALL_RECURRENT_SPAN_UPLOAD_FIXTURE_ONLY
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
  assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
  PioneerQ38SpanLease span;UploadIO io{*receipt,span,prepared,order,&context,hashes.data(),ledger.data(),0,mode};
  PioneerQ38SmallRecurrentSpanUpload<FakePrepared,UploadIO> upload;std::string error;
  const bool ok=upload.Run(prepared,*canonical,io,7,error);
  if(mode==0){
   assert(ok && error.empty() && upload.CompleteHeld() && upload.DeviceTouched() && !upload.Failed());
   assert(io.calls==21475 && upload.UploadedSpans()==21475 && upload.UploadedBytes()==UINT64_C(20754878464));
   span.closed=true;span.ready=span.dirty=false;assert(receipt->Seal(span));
  }else assert(!ok && !error.empty() && upload.DeviceTouched() && upload.Failed() && !upload.CompleteHeld() && io.calls==1);
  assert(!upload.Run(prepared,*canonical,io,7,error) && io.calls==(mode?1U:21475U));
 }
 {
  BanksMock backend;auto proof=BankProof(backend);PioneerResidentBanks banks;assert(banks.Bind(proof));
  std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
  auto receipt=std::make_unique<PioneerQ38SmallRecurrentUploadReceiptInspection>();int context=0,cookie=0;
  assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
  PioneerQ38SpanLease span;UploadIO io{*receipt,span,prepared,order,&context,hashes.data(),ledger.data(),0,0};
  PioneerQ38SmallRecurrentSpanUpload<FakePrepared,UploadIO> upload;std::string error;
  prepared.failAt=prepared.calls+1;
  assert(!upload.Run(prepared,*canonical,io,7,error) && upload.Failed() && !upload.DeviceTouched() && !io.calls);
  prepared.failAt=0;
  assert(!upload.Run(prepared,*canonical,io,7,error) && !io.calls);
 }
 puts("PASS prepared recurrent spans through Work wire protocol and matching receipt; uncertain calls and replies stop; SIMULATED NO DEVICE/SPEED");
}
#endif
