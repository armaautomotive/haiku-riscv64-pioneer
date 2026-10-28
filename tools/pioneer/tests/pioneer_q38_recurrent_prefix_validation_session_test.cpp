/* SPDX-License-Identifier: MIT */
// Actual client over SYNTHETIC raw extents/bytes and OS replies only.
// COW forks inject faults at each genuinely reached call without replaying
// fake progress or recomputing earlier uploads in a child.
#define PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38RecurrentPrefixValidationSession.h"
#include <cassert>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>
struct PrefixClientFaultSweep {
 bool enabled=true,child=false;unsigned fence=0,clones=0;const unsigned* calls=nullptr;
 template<class Change>bool Clone(Change change){
  if(!enabled)return false;
  assert(!child && calls);fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){child=true;fence=*calls;change();return true;}
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));clones++;return false;
 }
};
struct PrefixClientSource {
 PrefixClientFaultSweep& sweep;std::vector<PioneerRecurrentPrefixValidationExtent> extents;
 bool valid=true,prepared=false,sealed=false,gpu=false,returned=false,prepareFail=false,sealFail=false;
 unsigned reads=0,observers=0,extentCalls=0,badReport=0;
 uint64_t fingerprint[3]={0xbb911e57059b179fULL,0x0ccdfab72b2dd8e6ULL,0x09ade8ca16dc4749ULL};
 explicit PrefixClientSource(PrefixClientFaultSweep& f):sweep(f){
  const unsigned routes[]={786,978,1170,785,977,1169,1361};unsigned next[7];for(auto& n:next)n=0x04020000;
  extents.push_back({0,786,0x04000000,46080,12});
  for(unsigned i=0;i<168;i++){
   unsigned bank=i%7,pages=i==167?34765:1024,pad=i<17?4092:i==17?1476:0;
   extents.push_back({bank,routes[bank],next[bank],pages*4096-pad,pages});next[bank]+=pages*4096+4096;
  }
  uint64_t bytes=0;unsigned pages=0;for(const auto& e:extents){bytes+=e.bytes;pages+=e.pages;}
  assert(extents.size()==169 && bytes==842821248 && pages==205785);
 }
 bool Ready()const{return valid && prepared && sealed;}
 bool Prepare(){assert(!gpu && !prepared);prepared=!prepareFail;return prepared;}
 bool SealBeforeNativeContext(){assert(!gpu && prepared && !sealed);sealed=!sealFail;return sealed;}
 uint64_t ReferenceReads()const{return 11557;}
 uint64_t ReferenceFingerprint(unsigned layer)const{assert(layer<3);return fingerprint[layer];}
 uint64_t Bytes()const{return 842821248;}unsigned Pages()const{return 205785;}unsigned Spans()const{return 169;}
 bool Extent(unsigned i,PioneerRecurrentPrefixValidationExtent& out){
  assert(!gpu && Ready() && i<169);extentCalls++;out=extents[i];
  if(sweep.Clone([&]{out.route^=1;}))return true;return true;
 }
 unsigned Find(unsigned route,unsigned address,unsigned bytes)const{
  for(unsigned i=0;i<extents.size();i++){const auto& e=extents[i];
   if(e.route==route && address>=e.address && uint64_t(address)-e.address+bytes<=e.bytes)return i;
  }return ~0U;
 }
 bool Contains(unsigned route,unsigned address,unsigned bytes)const{return Find(route,address,bytes)!=~0U;}
 bool Read(unsigned route,unsigned address,uint8_t* output,unsigned bytes){
  assert(gpu && Ready() && bytes && bytes<=4096 && output);unsigned i=Find(route,address,bytes);assert(i!=~0U);reads++;
  if((reads==1 || reads==12 || reads==13 || reads==100000 || reads==205785) && sweep.Clone([]{}))return false;
  memset(output,int(i+1),bytes);return true;
 }
 bool CompareFinal(const uint32_t* values,unsigned words,PioneerRecurrentOriginalComparison* out){
  assert(gpu && returned && Ready() && words==5120 && out);observers++;*out={};out->checked=words;out->passed=1;
  if(badReport==1)out->checked--;if(badReport==2)out->passed=0;if(badReport==3)out->differences=1;
  if(badReport==4)out->nonfinite=1;if(badReport==5)out->maxAbsolute=1;if(badReport==6)out->relativeL2=1;
  return values[0]==0x54455354 && values[5119]==0x454e4421;
 }
};
struct PrefixClientIO {
 PrefixClientSource& source;PrefixClientFaultSweep& sweep;
 unsigned opens=0,closes=0,calls=0,begins=0,spans=0,spanEnds=0,runs=0,ends=0,uploaded=0,partAt=0,pageAt=0,expectedSpans=0;
 uint64_t replyBuildTag=TT_Q38_RECURRENT_PREFIX_VALIDATION_BUILD_TAG;unsigned replyNamespace=24;
 bool openFail=false,closeFail=false;bool(*earlyFinish)(void*)=nullptr;void* client=nullptr;
 explicit PrefixClientIO(PrefixClientSource& s):source(s),sweep(s.sweep){
  sweep.calls=&calls;for(const auto& e:s.extents)expectedSpans+=(e.pages+255)/256;
 }
 int Open(){assert(source.Ready() && source.extentCalls==169 && !source.gpu && !opens++);if(openFail)return -1;source.gpu=true;return 73;}
 int Close(int fd){assert(fd==73 && !closes++);source.gpu=false;return closeFail?-1:0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==73 && source.Ready() && source.gpu && !closes);calls++;
  if(earlyFinish){unsigned before=calls;assert(!earlyFinish(client) && calls==before);}
  switch(op){
   case TT_PCI_PROBE_RELOAD_STATUS:{assert(bytes==sizeof(tt_pioneer_driver_reload));auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
    assert(r.version==1);r.driver=55;r.guard_version=1;r.generation=4;r.open_count=1;r.build_tag=replyBuildTag;break;}
   case TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATION_SUPPORT:
    assert(bytes==sizeof(tt_q38_embedding_support));*static_cast<tt_q38_embedding_support*>(data)={1,55,replyNamespace,5120,248320,512,0,0};break;
   case TT_PCI_PROBE_Q38_BANK_SUPPORT:*static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,UINT64_C(3)*1024*1024*1024};break;
   case TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT:*static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};break;
   case TT_PCI_PROBE_Q38_BANK_BEGIN:{begins++;auto& r=static_cast<tt_q38_bank_session*>(data)->result;assert(!source.observers);
    r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=UINT64_C(21)*1024*1024*1024;break;}
   case TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD:{auto& r=*static_cast<tt_q38_span_upload*>(data);assert(partAt<169);const auto& e=source.extents[partAt];
    unsigned pages=std::min(256U,e.pages-pageAt);assert(bytes==sizeof(r) && begins==1 && !runs && !spanEnds && !source.observers
     && r.version==1 && !r.reserved && r.bank==e.bank && r.offset==e.address-0x04000000U+uint64_t(pageAt)*4096 && r.bytes==pages*4096);
    for(unsigned page=0;page<pages;page++){
     const auto* raw=reinterpret_cast<const uint8_t*>(r.data+page*1024);unsigned count=std::min(4096U,e.bytes-(pageAt+page)*4096);
     assert(raw[0]==partAt+1 && raw[count-1]==partAt+1);for(unsigned tail=count;tail<4096;tail++)assert(!raw[tail]);
    }
    spans++;uploaded+=pages;pageAt+=pages;if(pageAt==e.pages){partAt++;pageAt=0;}
    r.result={1,TT_RUNTIME_PASSED,pages*4096,pages,spans,1,1,1,0,0,0,0};break;}
   case TT_PCI_PROBE_Q38_SPAN_END:assert(partAt==169 && !pageAt && uploaded==205785 && spans==expectedSpans && !runs);
    spanEnds++;*static_cast<tt_q38_span_end*>(data)={1,0,1,0};break;
   case TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE:{runs++;auto& r=*static_cast<tt_q38_recurrent_prefix_validation*>(data);
    assert(bytes==sizeof(r) && PioneerQ38RecurrentPrefixValidationRequest(r) && spanEnds==1 && !source.observers);
    const uint32_t header[96]={1,0,0,0,TT_RUNTIME_PASSED,1,1,0,7,0,4,0,123,0,205785,512,11480641,11480641,
     2,1,672,336,73728,1680,1,119,119,120,1,1,57,11406904,22813808,2633615,11406904,5,5120,5120,3,24,4,TT_RESIDENT_SESSION_CLOSED,1,1,0};
    memcpy(&r,header,sizeof(header));r.values[0]=0x54455354;r.values[5119]=0x454e4421;source.returned=true;break;}
   case TT_PCI_PROBE_Q38_BANK_END:ends++;assert(!"NEVER duplicate BANK_END after complete-END reply");break;
   default:assert(!"no legacy layer0/dense/Work/address fallback");
  }
  if(sweep.Clone([]{}))return false; // actual transport failure at EVERY reached call
  if(calls==1 || calls==5 || calls==6 || calls==406 || calls==811 || calls==812){
   if(sweep.Clone([&]{source.valid=false;}))return true;
   for(unsigned layer=0;layer<3;layer++)if(sweep.Clone([&]{source.fingerprint[layer]++;}))return true;
  }
  if(op==TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE){
   for(unsigned i=0;i<96;i++)if(i!=9 && i!=13){
    if(sweep.Clone([&]{auto* words=static_cast<uint32_t*>(data);if(i==8 || i==12)words[i]=0;else words[i]^=1;}))return true;
   }
   for(unsigned metric=1;metric<=6;metric++)if(sweep.Clone([&]{source.badReport=metric;}))return true;
  }
  return true;
 }
};
using PrefixSession=PioneerQ38RecurrentPrefixValidationSession<PrefixClientIO,PrefixClientSource>;
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);unsigned clones=0,calls=0;
 {
  PrefixClientFaultSweep sweep;PrefixClientSource source(sweep);PrefixClientIO io(source);auto session=std::make_unique<PrefixSession>(io,source);
  io.client=session.get();io.earlyFinish=[](void* p){return static_cast<PrefixSession*>(p)->Finish();};
  assert(!session->Finish() && !session->Validate(1) && !session->Validate(0,1) && !io.opens);
  bool okay=session->Validate();
  if(sweep.child){
   assert(!okay && io.calls==sweep.fence && !io.ends && io.closes==unsigned(io.opens && !io.openFail));
   auto before=io.calls,closed=io.closes,observed=source.observers;session->Abort();session->Abort();
   assert(!session->Validate() && !session->Finish() && io.calls==before && io.closes==closed && source.observers==observed);_exit(0);
  }
  assert(okay && source.reads==205785 && source.observers==1 && io.spans==805 && io.calls==812 && !io.closes && !io.ends);
  assert(session->Finish() && io.closes==1 && !session->Finish() && !session->Validate() && !io.ends);
  clones=sweep.clones;calls=io.calls;
 }
 unsigned early=0;
 for(uint64_t tag:{UINT64_C(0),UINT64_C(0x2026102700010072),UINT64_C(0x2026102700010071),UINT64_C(0x2026102100010012)}){
  PrefixClientFaultSweep sweep;sweep.enabled=false;PrefixClientSource source(sweep);PrefixClientIO io(source);io.replyBuildTag=tag;
  auto s=std::make_unique<PrefixSession>(io,source);assert(!s->Validate() && !strcmp(s->failure,"driver_identity")
   && io.calls==1 && io.closes==1 && !io.begins && !io.spans && !io.runs && !io.ends);early++;
 }
 for(unsigned modelNamespace:{0U,21U,22U,23U}){
  PrefixClientFaultSweep sweep;sweep.enabled=false;PrefixClientSource source(sweep);PrefixClientIO io(source);io.replyNamespace=modelNamespace;
  auto s=std::make_unique<PrefixSession>(io,source);assert(!s->Validate() && !strcmp(s->failure,"diagnostic_identity")
   && io.calls==2 && io.closes==1 && !io.begins && !io.spans && !io.runs && !io.ends);early++;
 }
 for(unsigned bad=0;bad<4;bad++){
  PrefixClientFaultSweep sweep;sweep.enabled=false;PrefixClientSource source(sweep);PrefixClientIO io(source);
  if(bad==0)source.prepareFail=true;if(bad==1)source.sealFail=true;if(bad==2)io.openFail=true;if(bad==3)io.closeFail=true;
  auto s=std::make_unique<PrefixSession>(io,source);bool okay=s->Validate();assert(!(okay && s->Finish()) && !io.ends);
  auto before=io.calls,closed=io.closes;s->Abort();assert(!s->Validate() && !s->Finish() && io.calls==before && io.closes==closed);early++;
 }
 printf("PASS DISTINCT prefix client/seal+169RAW extents BEFORE FD/205785pages/805uploads/%ucalls/ALL raw tails zero/strict last-layer comparison AFTER complete-END reply/no duplicate END;SYNTHETIC endpoints ONLY\n",calls);
 printf("PASS actual client%u COW reached faults=ALL%u transport calls+169extent refusals+5raw reads+24source/3layer identity losses+94header corruptions+6strict metrics;%u early tag/namespace/prepare/seal/open/close refusals;terminal CLOSE ONLY,ZERO following calls/retry/END;NOT original numerical,native,physical,Work or TPS proof\n",clones,calls,early);
}
