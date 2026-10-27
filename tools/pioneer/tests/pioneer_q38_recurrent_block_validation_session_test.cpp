/* SPDX-License-Identifier: MIT */
// Client protocol/termination over explicitly SYNTHETIC extents+raw bytes
// and OS replies. NOT full-SHA/original arithmetic/native/silicon proof.
#define PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38RecurrentBlockValidationSession.h"
#include <cassert>
#include <cstdio>
struct RecurrentClientSource {
 std::vector<PioneerRecurrentOriginalExtent> extents;
 bool valid=true,prepared=false,sealed=false,gpu=false,returned=false,prepareFail=false,sealFail=false;
 bool observerFail=false,observerDrop=false;unsigned reads=0,failRead=0,observers=0,extentCalls=0,badExtent=~0U,badReport=0;
 uint64_t fingerprint=0xbb911e57059b179fULL;
 RecurrentClientSource(){
  const unsigned routes[]={786,978,1170,785,977,1169,1361};unsigned next[7];for(auto& n:next)n=0x04020000;
  extents.push_back({0,786,0x04000000,46080,12});
  for(unsigned i=0;i<56;i++){
   unsigned bank=i%7,pages=i==55?12271:1024,pad=i<5?4092:i==5?3220:0;
   extents.push_back({bank,routes[bank],next[bank],pages*4096-pad,pages});next[bank]+=pages*4096+4096;
  }
  uint64_t bytes=0;unsigned pages=0;for(const auto& e:extents){bytes+=e.bytes;pages+=e.pages;}
  assert(bytes==280971136 && pages==68603);
 }
 bool Ready()const{return valid && prepared && sealed;}
 bool Prepare(){assert(!gpu && !prepared);prepared=!prepareFail;return prepared;}
 bool SealBeforeNativeContext(){assert(!gpu && prepared && !sealed);sealed=!sealFail;return sealed;}
 uint64_t ReferenceReads()const{return 3853;}
 uint64_t ReferenceFingerprint()const{return fingerprint;}
 uint64_t Bytes()const{return 280971136;}unsigned Pages()const{return 68603;}unsigned Spans()const{return 57;}
 bool Extent(unsigned i,PioneerRecurrentOriginalExtent& output){
  assert(!gpu && Ready() && i<57);extentCalls++;output=extents[i];if(i==badExtent)output.route^=1;return true;
 }
 unsigned Find(unsigned route,unsigned address,unsigned bytes)const{
  for(unsigned i=0;i<extents.size();i++){const auto& e=extents[i];
   if(e.route==route && address>=e.address && uint64_t(address)-e.address+bytes<=e.bytes)return i;
  }return ~0U;
 }
 bool Contains(unsigned route,unsigned address,unsigned bytes)const{return Find(route,address,bytes)!=~0U;}
 bool Read(unsigned route,unsigned address,uint8_t* output,unsigned bytes){
  assert(gpu && Ready() && bytes && bytes<=4096 && output);unsigned i=Find(route,address,bytes);assert(i!=~0U);
  if(++reads==failRead)return false;memset(output,int(i+1),bytes);return true;
 }
 bool CompareStage(PioneerRecurrentOriginalStage stage,const uint32_t* values,unsigned words,PioneerRecurrentOriginalComparison* out){
  assert(gpu && returned && Ready() && stage==PioneerRecurrentFinal && words==5120 && out);observers++;
  *out={};out->checked=words;out->passed=1;
  if(badReport==1)out->checked--;if(badReport==2)out->passed=0;if(badReport==3)out->differences=1;
  if(badReport==4)out->nonfinite=1;if(badReport==5)out->maxAbsolute=1;if(badReport==6)out->relativeL2=1;
  if(observerDrop)valid=false;
  return !observerFail && values[0]==0x54455354 && values[5119]==0x454e4421;
 }
};
struct RecurrentClientIO {
 RecurrentClientSource& source;
 unsigned opens=0,closes=0,calls=0,begins=0,spans=0,spanEnds=0,runs=0,ends=0,uploaded=0,partAt=0,pageAt=0,expectedSpans=0;
 unsigned failCall=0,sourceDrop=0,identityDrop=0,badHeader=~0U,badCall=0,badOffset=0;
 uint64_t replyBuildTag=UINT64_C(0x2026102700010072);unsigned replyNamespace=23;
 bool openFail=false,closeFail=false;bool(*earlyFinish)(void*)=nullptr;void* client=nullptr;
 explicit RecurrentClientIO(RecurrentClientSource& s):source(s){for(const auto& e:s.extents)expectedSpans+=(e.pages+255)/256;}
 int Open(){assert(source.Ready() && source.extentCalls==57 && !source.gpu && !opens++);if(openFail)return -1;source.gpu=true;return 71;}
 int Close(int fd){assert(fd==71 && !closes++);source.gpu=false;return closeFail?-1:0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==71 && source.Ready() && source.gpu && !closes);calls++;
  if(earlyFinish){unsigned before=calls;assert(!earlyFinish(client) && calls==before);}
  switch(op){
   case TT_PCI_PROBE_RELOAD_STATUS:{assert(bytes==sizeof(tt_pioneer_driver_reload));auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
    assert(r.version==1);r.driver=55;r.guard_version=1;r.generation=4;r.open_count=1;r.build_tag=replyBuildTag;break;}
   case TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATION_SUPPORT:
    assert(bytes==sizeof(tt_q38_embedding_support));*static_cast<tt_q38_embedding_support*>(data)={1,55,replyNamespace,5120,248320,512,0,0};break;
   case TT_PCI_PROBE_Q38_BANK_SUPPORT:*static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,UINT64_C(3)*1024*1024*1024};break;
   case TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT:*static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};break;
   case TT_PCI_PROBE_Q38_BANK_BEGIN:{begins++;auto& r=static_cast<tt_q38_bank_session*>(data)->result;assert(!source.observers);
    r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=UINT64_C(21)*1024*1024*1024;break;}
   case TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD:{auto& r=*static_cast<tt_q38_span_upload*>(data);assert(partAt<57);const auto& e=source.extents[partAt];
    unsigned pages=std::min(256U,e.pages-pageAt);assert(bytes==sizeof(r) && begins==1 && !runs && !spanEnds && !source.observers
     && r.version==1 && !r.reserved && r.bank==e.bank && r.offset==e.address-0x04000000U+uint64_t(pageAt)*4096 && r.bytes==pages*4096);
    for(unsigned page=0;page<pages;page++){
     const auto* raw=reinterpret_cast<const uint8_t*>(r.data+page*1024);unsigned count=std::min(4096U,e.bytes-(pageAt+page)*4096);
     assert(raw[0]==partAt+1 && raw[count-1]==partAt+1);for(unsigned tail=count;tail<4096;tail++)assert(!raw[tail]);
    }
    spans++;uploaded+=pages;pageAt+=pages;if(pageAt==e.pages){partAt++;pageAt=0;}
    r.result={1,TT_RUNTIME_PASSED,pages*4096,pages,spans,1,1,1,0,0,0,0};break;}
   case TT_PCI_PROBE_Q38_SPAN_END:assert(partAt==57 && !pageAt && uploaded==68603 && spans==expectedSpans && !runs);
    spanEnds++;*static_cast<tt_q38_span_end*>(data)={1,0,1,0};break;
   case TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE:{runs++;auto& r=*static_cast<tt_q38_recurrent_block_validation*>(data);
    assert(bytes==sizeof(r) && PioneerQ38RecurrentBlockValidationRequest(r) && spanEnds==1 && !source.observers);
    const uint32_t header[80]={1,0,0,0,TT_RUNTIME_PASSED,1,1,0,7,0,4,0,123,0,68603,512,11480641,11480641,
     2,1,672,336,73728,1680,1,119,119,120,1,1,25,11406904,22813808,2632668,11406904,5,5120,5120};
    memcpy(&r,header,sizeof(header));r.values[0]=0x54455354;r.values[5119]=0x454e4421;
    if(badHeader<80){auto* w=reinterpret_cast<uint32_t*>(&r);if(badHeader==8 || badHeader==12)w[badHeader]=0;else w[badHeader]^=1;}
    source.returned=true;break;}
   case TT_PCI_PROBE_Q38_BANK_END:{ends++;assert(source.observers==1 && runs==1);auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    r.version=1;r.state=TT_RESIDENT_SESSION_CLOSED;r.host_released=r.pci_restored=1;r.bytes=UINT64_C(21)*1024*1024*1024;break;}
   default:assert(!"no legacy Work/old DEBUG/address fallback");
  }
  if(calls==badCall){assert(badOffset+4<=bytes);auto* word=reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(data)+badOffset);*word^=1;}
  if(calls==sourceDrop)source.valid=false;if(calls==identityDrop)source.fingerprint++;
  return calls!=failCall;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);unsigned totalCalls=0,failures=0;
 using Session=PioneerQ38RecurrentBlockValidationSession<RecurrentClientIO,RecurrentClientSource>;
 {RecurrentClientSource source;RecurrentClientIO io(source);auto s=std::make_unique<Session>(io,source);
  io.client=s.get();io.earlyFinish=[](void* p){return static_cast<Session*>(p)->Finish();};
  assert(!s->Finish() && !s->Validate(1) && !s->Validate(0,1) && !io.opens);
  assert(s->Validate() && source.reads==68603 && source.observers==1);io.earlyFinish=nullptr;
  assert(s->Finish() && io.ends==1 && io.closes==1);totalCalls=io.calls;
  assert(!s->Validate() && !s->Finish());printf("PASS DEBUG72 actual client seal+57RAW extent checks BEFORE FD/68603pages/%uuploads/%ucalls/ALL raw tails zero/strict comparison BEFORE explicit END;SYNTHETIC endpoints ONLY\n",io.spans,totalCalls);
 }
 for(uint64_t tag:{UINT64_C(0),UINT64_C(0x2026102700010071),UINT64_C(0x2026102700010070),UINT64_C(0x2026102100010012)}){
  RecurrentClientSource source;RecurrentClientIO io(source);io.replyBuildTag=tag;auto s=std::make_unique<Session>(io,source);
  assert(!s->Validate() && !strcmp(s->failure,"driver_identity") && io.opens==1 && io.closes==1 && io.calls==1
   && !io.begins && !io.spans && !io.runs && !io.ends);
  s->Abort();assert(!s->Validate() && !s->Finish() && io.calls==1 && io.closes==1);failures++;
 }
 for(unsigned modelNamespace:{0U,21U,22U,24U}){
  RecurrentClientSource source;RecurrentClientIO io(source);io.replyNamespace=modelNamespace;auto s=std::make_unique<Session>(io,source);
  assert(!s->Validate() && !strcmp(s->failure,"diagnostic_identity") && io.opens==1 && io.closes==1 && io.calls==2
   && !io.begins && !io.spans && !io.runs && !io.ends);
  s->Abort();assert(!s->Validate() && !s->Finish() && io.calls==2 && io.closes==1);failures++;
 }
 puts("PASS DEBUG72 rejects8 old/foreign buildtag+namespace pairs BEFORE BANK_BEGIN/upload/compute;no legacy71/22 fallback");
 auto check=[&](unsigned kind,unsigned at){
  RecurrentClientSource source;RecurrentClientIO io(source);
  if(kind==0)io.failCall=at;if(kind==1)io.sourceDrop=at;if(kind==2)io.identityDrop=at;if(kind==3)source.failRead=at;
  if(kind==4)io.badHeader=at;if(kind==5)source.observerFail=true;if(kind==6)source.observerDrop=true;
  if(kind==7)source.badReport=at;if(kind==8)source.prepareFail=true;if(kind==9)source.sealFail=true;
  if(kind==10)io.openFail=true;if(kind==11)io.closeFail=true;if(kind==12)source.badExtent=at;
  {auto s=std::make_unique<Session>(io,source);bool okay=s->Validate();bool clean=okay && s->Finish();assert(!clean);
   unsigned calls=io.calls,observers=source.observers;s->Abort();s->Abort();
   assert(!s->Finish() && !s->Validate() && io.calls==calls && observers==source.observers);if(calls<totalCalls)assert(!io.ends);
  }
  assert(io.closes==unsigned(io.opens && !io.openFail));if(kind==12)assert(!io.opens && !io.calls);failures++;
 };
 for(unsigned at=1;at<=totalCalls;at++){check(0,at);if(!(at%64))printf("DEBUG72_CLIENT_FAULT_PROGRESS %u/%u\n",at,totalCalls);}
 for(unsigned at:{1U,5U,6U,totalCalls/2,totalCalls-2,totalCalls-1,totalCalls}){check(1,at);check(2,at);}
 for(unsigned at:{1U,12U,13U,32768U,68603U})check(3,at);
 for(unsigned at=0;at<80;at++)if(at!=9 && at!=13)check(4,at);
 for(unsigned kind:{5U,6U,8U,9U,10U,11U})check(kind,0);
 for(unsigned metric=1;metric<=6;metric++)check(7,metric);
 for(unsigned extent=0;extent<57;extent++)check(12,extent);
 printf("PASS DEBUG72 actual client%u failure cases/ALL%ucalls/source+identity loss/partial raw reads/78header corruptions/6strict metrics/ALL57extent refusals BEFORE FD/open-close-prepare-seal;terminal CLOSE ONLY,no END/reset/peek/retry after uncertainty;NOT original numerical/native/silicon/TPS proof\n",failures,totalCalls);
}
