/* SPDX-License-Identifier: MIT */
// Actual span-wire packets and full fixed hybrid geometry over fake endpoints.
// No accelerator, OS model, learned data, driver authority or inference.
#define PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38HybridSpanUpload.h"
#include <cassert>
#include <cstdio>
#include <memory>

using Canonical=PioneerQ38CanonicalModelPlan;
struct FakePages {
 PioneerQ38HybridModelPlan::Proposal proposal;bool live=true;unsigned reads=0,failRead=0;
 explicit FakePages(const Canonical& c){assert(PioneerQ38HybridModelPlan::Build(c,UINT64_C(0x1fff000000000000),proposal));}
 bool AllVerified()const{return live;}
 const PioneerQ38HybridModelPlan::Part* Part(unsigned i)const{return live && i<proposal.parts.size()?&proposal.parts[i]:nullptr;}
 bool ReadPage(unsigned part,unsigned page,uint8_t* out,unsigned capacity,std::string& error){
  if(!live || part>=proposal.parts.size() || page>=proposal.parts[part].allocated/4096 || capacity!=4096 || ++reads==failRead){
   error="injected page failure";return false;
  }
  std::memset(out,int((part+page)%251+1),4096);return true;
 }
 bool ReadPages(unsigned part,unsigned page,unsigned count,uint8_t* out,unsigned capacity,std::string& error){
  if(!count || count>16 || capacity<count*4096)return false;
  for(unsigned i=0;i<count;i++)if(!ReadPage(part,page+i,out+i*4096,4096,error))return false;
  return true;
 }
};
struct FakeIO {
 const Canonical& canonical;FakePages& pages;unsigned calls=0,fragment=0,page=0,failCall=0,badField=0;
 bool revoke=false,requestFault=false;uint64_t transferred=0;
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==42 && op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && size==sizeof(tt_q38_span_upload));
  calls++;if(calls==failCall)return false;
  while(fragment<canonical.Count() && canonical.Part(fragment)->kind!=Canonical::WeightKind)fragment++;
  assert(fragment<canonical.Count());const auto& p=pages.proposal.parts[fragment];
  auto& q=*static_cast<tt_q38_span_upload*>(data);
  const unsigned count=std::min(256U,p.allocated/4096-page);
  assert(q.version==1 && !q.reserved && q.bank==p.bank && q.offset==uint64_t(p.offset)+uint64_t(page)*4096 && q.bytes==count*4096);
  for(unsigned i=0;i<count;i++){
   const auto* raw=reinterpret_cast<const uint8_t*>(q.data)+i*4096;
   assert(raw[0]==(fragment+page+i)%251+1 && raw[4095]==raw[0]);
  }
  transferred+=q.bytes;page+=count;if(page==p.allocated/4096){fragment++;page=0;}
  q.result={1,TT_RUNTIME_PASSED,q.bytes,count,calls,1,1,1,0,0,0,0};
  if(badField){
   uint32_t* fields[]={&q.result.version,&q.result.state,&q.result.bytes,&q.result.pages,&q.result.nonce,
    &q.result.started,&q.result.completed,&q.result.held,&q.result.host_exact,&q.result.matched_words,
    &q.result.recovery_required,&q.result.reserved};
   assert(badField<=12);*fields[badField-1]^=1;
  }
  if(requestFault)q.offset+=4096;
  if(revoke)pages.live=false;
  return true;
 }
};
#ifndef PIONEER_Q38_HYBRID_SPAN_FIXTURE_ONLY
int main(){
 auto canonical=std::make_unique<Canonical>();assert(canonical->Build());std::string error;
 using Upload=PioneerQ38HybridSpanUpload<FakePages,FakeIO>;
 {
  FakePages pages(*canonical);FakeIO io{*canonical,pages};Upload upload;pages.live=false;
  assert(!upload.Run(pages,*canonical,io,42,error) && upload.Failed() && !upload.DeviceTouched() && !io.calls);
  pages.live=true;assert(!upload.Run(pages,*canonical,io,42,error) && !io.calls);
 }
 {
  FakePages pages(*canonical);FakeIO io{*canonical,pages};Upload upload;pages.proposal.parts[0].offset+=4096;
  assert(!upload.Run(pages,*canonical,io,42,error) && upload.Failed() && !upload.DeviceTouched() && !io.calls);
 }
 // The BFP4 fragment has the same length as original Q4, but must not be
 // accepted with an original-format tag (or vice versa).
 for(unsigned wanted:{unsigned(PioneerQ38HybridModelPlan::Original),unsigned(PioneerQ38HybridModelPlan::Bfp4),
                      unsigned(PioneerQ38HybridModelPlan::Bfp8)}){
  FakePages pages(*canonical);FakeIO io{*canonical,pages};Upload upload;unsigned index=0;
  while(index<pages.proposal.parts.size() && pages.proposal.parts[index].encoding!=wanted)index++;
  assert(index<pages.proposal.parts.size());
  pages.proposal.parts[index].encoding=wanted==PioneerQ38HybridModelPlan::Bfp4
   ?PioneerQ38HybridModelPlan::Original:PioneerQ38HybridModelPlan::Bfp4;
  assert(!upload.Run(pages,*canonical,io,42,error) && upload.Failed() && !upload.DeviceTouched() && !io.calls);
 }
 {
  FakePages pages(*canonical);FakeIO io{*canonical,pages};Upload upload;pages.failRead=1;
  assert(!upload.Run(pages,*canonical,io,42,error) && upload.Failed() && !upload.DeviceTouched() && !io.calls);
 }
 {
  FakePages pages(*canonical);FakeIO io{*canonical,pages};Upload upload;pages.failRead=257;
  assert(!upload.Run(pages,*canonical,io,42,error) && upload.Failed() && upload.DeviceTouched() && io.calls==1);
 }
 {
  FakePages pages(*canonical);FakeIO io{*canonical,pages};Upload upload;io.failCall=2;
  assert(!upload.Run(pages,*canonical,io,42,error) && upload.Failed() && upload.DeviceTouched() && io.calls==2);
 }
 for(unsigned mode=0;mode<15;mode++){
  FakePages pages(*canonical);FakeIO io{*canonical,pages};Upload upload;
  if(mode<12)io.badField=mode+1;else if(mode==12)io.failCall=1;else if(mode==13)io.requestFault=true;else io.revoke=true;
  assert(!upload.Run(pages,*canonical,io,42,error) && upload.Failed() && upload.DeviceTouched()
   && !upload.CompleteHeld() && io.calls==1);
  const auto calls=io.calls;assert(!upload.Run(pages,*canonical,io,42,error) && io.calls==calls);
 }
 {
  FakePages pages(*canonical);FakeIO io{*canonical,pages};Upload upload;
  assert(!upload.Run(pages,*canonical,io,42,error,[&](uint64_t,uint64_t){pages.live=false;})
   && upload.Failed() && upload.DeviceTouched() && io.calls==1);
 }
 {
  FakePages pages(*canonical);FakeIO io{*canonical,pages};Upload upload;uint64_t planned=0;
  for(unsigned i=0;i<canonical->Count();i++)if(canonical->Part(i)->kind==Canonical::WeightKind)planned+=pages.proposal.parts[i].allocated;
  unsigned progress=0;
  assert(upload.Run(pages,*canonical,io,42,error,[&](uint64_t done,uint64_t total){
   assert(done==io.transferred && total==planned);progress++;
  }));
  assert(upload.CompleteHeld() && !upload.Failed() && upload.DeviceTouched() && upload.UploadedParts()==3551
   && upload.UploadedBytes()==planned && upload.PlannedBytes()==planned && io.transferred==planned
   && io.calls==upload.UploadedSpans() && progress==io.calls && pages.reads==planned/4096);
  printf("PASS full synthetic hybrid upload: %u fragments, %u real-format span packets, %llu bytes, %u pages; "
   "held only, NO device access, model data, inference or performance result\n",
   upload.UploadedParts(),upload.UploadedSpans(),(unsigned long long)planned,pages.reads);
 }
 puts("PASS preflight mismatch, read failure, 12 reply fields, uncertain call, changed request, revoked source and no retry");
}
#endif
