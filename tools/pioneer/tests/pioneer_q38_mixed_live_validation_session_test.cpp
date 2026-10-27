/* SPDX-License-Identifier: MIT */
// Actual client protocol over SYNTHETIC file schema/raw pages/OS replies.
// Does NOT authenticate original bytes or prove arithmetic/silicon speed.
#define PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38MixedLiveValidationSession.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
struct DenseSessionSource {
 PioneerQwen38Gguf model;FILE* file;bool valid=true,prepared=false,sealed=false,gpu=false,returned=false;
 bool prepareFail=false,sealFail=false,observerFail=false,observerDrop=false,badReport=false;
 unsigned reads=0,failRead=0,observers=0;
 explicit DenseSessionSource(FILE* f):file(f){assert(model.Open(file));}
 bool Ready()const{return valid && prepared && sealed && model.Stable();}
 bool Prepare(){assert(!gpu && !prepared);prepared=!prepareFail;return prepared;}
 bool SealBeforeNativeContext(){assert(!gpu && prepared && !sealed);sealed=!sealFail;return sealed;}
 const PioneerQwen38Gguf* SealedModel()const{return Ready()?&model:nullptr;}
 uint64_t ReferenceReads()const{return 3717;}
 uint64_t ReferenceFingerprint()const{return 0xec4e50bb1a601765ULL;}
 bool ReadWeightPage(unsigned part,unsigned page,uint8_t* bytes,unsigned capacity){
  assert(gpu && Ready() && capacity==4096);if(++reads==failRead)return false;
  memset(bytes,0,4096);uint32_t marker[]={0x53594e54,part,page,~page};memcpy(bytes,marker,sizeof(marker));
  // Deliberately nonzero adjacent embedding-tile bytes. Actual client
  // MUST clear these rather than leaking them into token0's padded page.
  if(!part && page==11)memset(bytes+1024,0x77,3072);
  return true;
 }
 bool CompareFinal(const uint32_t* values,unsigned words,PioneerQ38MixedLiveReferenceComparison* r){
  assert(gpu && returned && Ready() && words==5120 && r);observers++;
  *r={};r->checked=words;r->passed=1;
  if(badReport)r->differences=1;
  if(observerDrop)valid=false;
  return !observerFail && values[0]==0x54455354 && values[5119]==0x454e4421;
 }
};
struct DenseSessionIO {
 DenseSessionSource& source;PioneerQwen38PackedPlan plan;
 struct Part{unsigned index,pages;};std::vector<Part> selected;
 unsigned opens=0,closes=0,calls=0,begins=0,spans=0,spanEnds=0,runs=0,ends=0,uploaded=0,partAt=0,pageAt=0,expectedSpans=0;
 unsigned failCall=0,sourceDrop=0,epochDrop=0,badHeader=UINT32_MAX,badCall=0,badOffset=0;
 bool openFail=false,closeFail=false;bool(*earlyFinish)(void*)=nullptr;void* client=nullptr;
 explicit DenseSessionIO(DenseSessionSource& s):source(s){
  assert(plan.Build(source.model,8192,true,true));auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
  assert(plan.Parts().size()==canonical->Count());
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];const auto& c=*canonical->Part(i);
   assert(p.kind==c.kind && p.layer==c.layer && p.role==c.role && p.bank==c.bank && p.firstRow==c.firstRow
    && p.rows==c.rows && p.rowBytes==c.rowBytes && p.bytes==c.bytes && p.allocated==c.allocated && p.proposedAddress==canonical->Base+c.offset);
   if(p.kind!=PIONEER_Q38_WEIGHT)continue;
   if(p.layer==64 && !p.role && !p.firstRow)selected.insert(selected.begin(),{i,12});
   if(p.layer==3)selected.push_back({i,p.allocated/4096});
  }
  assert(selected.size()==54 && selected[0].index==0);
  for(const auto& p:selected)expectedSpans+=(p.pages+255)/256;
 }
 int Open(){assert(source.Ready() && !source.gpu && !opens++);if(openFail)return -1;source.gpu=true;return 73;}
 int Close(int fd){assert(fd==73 && !closes++);source.gpu=false;return closeFail?-1:0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==73 && source.Ready() && source.gpu && !closes);calls++;
  if(earlyFinish){unsigned before=calls;assert(!earlyFinish(client) && calls==before);}
  switch(op){
   case TT_PCI_PROBE_RELOAD_STATUS:{auto& r=*static_cast<tt_pioneer_driver_reload*>(data);assert(bytes==sizeof(r) && r.version==1);
    r.driver=55;r.guard_version=1;r.generation=4;r.open_count=1;r.build_tag=UINT64_C(0x2026102700010070);break;}
   case TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATION_SUPPORT:
    assert(bytes==sizeof(tt_q38_embedding_support));*static_cast<tt_q38_embedding_support*>(data)={1,55,21,5120,248320,512,0,0};break;
   case TT_PCI_PROBE_Q38_BANK_SUPPORT:*static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,PioneerResidentBanks::Capacity};break;
   case TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT:*static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};break;
   case TT_PCI_PROBE_Q38_BANK_BEGIN:{begins++;auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    assert(!source.observers);r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=PioneerResidentBanks::Total;break;}
   case TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD:{auto& r=*static_cast<tt_q38_span_upload*>(data);assert(partAt<selected.size());
    const auto part=selected[partAt];const auto& p=plan.Parts()[part.index];unsigned pages=std::min(256U,part.pages-pageAt);
    assert(bytes==sizeof(r) && begins==1 && !runs && !spanEnds && !source.observers && r.version==1 && !r.reserved
     && r.bank==p.bank && r.offset==p.proposedAddress-plan.Base+uint64_t(pageAt)*4096 && r.bytes==pages*4096);
    for(unsigned page=0;page<pages;page++){
     const auto* w=r.data+page*1024;assert(w[0]==0x53594e54 && w[1]==part.index && w[2]==pageAt+page && w[3]==~(pageAt+page));
     if(partAt==0 && pageAt+page==11)for(unsigned i=256;i<1024;i++)assert(!w[i]);
    }
    spans++;uploaded+=pages;pageAt+=pages;if(pageAt==part.pages){partAt++;pageAt=0;}
    r.result={1,TT_RUNTIME_PASSED,pages*4096,pages,spans,1,1,1,0,0,0,0};break;}
   case TT_PCI_PROBE_Q38_SPAN_END:assert(partAt==54 && !pageAt && uploaded==63266 && spans==expectedSpans && !runs);
    spanEnds++;*static_cast<tt_q38_span_end*>(data)={1,0,1,0};break;
   case TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE:{runs++;auto& r=*static_cast<tt_q38_mixed_live_validation*>(data);
    assert(bytes==sizeof(r) && PioneerQ38MixedLiveValidationRequest(r) && spanEnds==1 && !source.observers);
    const uint32_t header[64]={1,0,0,0,TT_RUNTIME_PASSED,1,1,0,7,0,4,0,123,0,63266,512,11472449,11472449,
     2,1,1,119,119,120,1,1,4,11398712,22797424,2630395,11398712,5,5120,5120};
    memcpy(&r,header,sizeof(header));r.values[0]=0x54455354;r.values[5119]=0x454e4421;
    if(badHeader<64){auto* w=reinterpret_cast<uint32_t*>(&r);if(badHeader==8 || badHeader==12)w[badHeader]=0;else w[badHeader]^=1;}
    source.returned=true;break;}
   case TT_PCI_PROBE_Q38_BANK_END:{ends++;assert(source.observers==1 && runs==1);auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    r.version=1;r.state=TT_RESIDENT_SESSION_CLOSED;r.host_released=r.pci_restored=1;r.bytes=PioneerResidentBanks::Total;break;}
   default:assert(!"no legacy Work/old DEBUG/address fallback");
  }
  if(calls==badCall){assert(badOffset+4<=bytes);auto* w=reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(data)+badOffset);*w^=1;}
  if(calls==sourceDrop)source.valid=false;
  if(calls==epochDrop)assert(source.model.Open(source.file));
  return calls!=failCall;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;unsigned totalCalls=0,failures=0;
 using Session=PioneerQ38MixedLiveValidationSession<DenseSessionIO,DenseSessionSource>;
 {DenseSessionSource source(fixture.file);DenseSessionIO io(source);Session s(io,source);
  io.client=&s;io.earlyFinish=[](void* p){return static_cast<Session*>(p)->Finish();};
  assert(!s.Finish() && !s.Validate(1) && !s.Validate(0,1) && !io.opens);
  assert(s.Validate() && source.reads==63266 && source.observers==1);io.earlyFinish=nullptr;
  assert(s.Finish() && io.ends==1 && io.closes==1);totalCalls=io.calls;
  assert(!s.Validate() && !s.Finish());printf("PASS DEBUG70 actual client original-source seal BEFORE FD/54raw spans/63266pages/%uuploads/%ucalls/zeroed embedding tail/compare BEFORE END;SYNTHETIC endpoints only\n",io.spans,totalCalls);
 }
 auto check=[&](unsigned kind,unsigned at){
  DenseSessionSource source(fixture.file);DenseSessionIO io(source);
  if(kind==0)io.failCall=at;if(kind==1)io.sourceDrop=at;if(kind==2)io.epochDrop=at;
  if(kind==3)source.failRead=at;if(kind==4)io.badHeader=at;
  if(kind==5)source.observerFail=true;if(kind==6)source.observerDrop=true;if(kind==7)source.badReport=true;
  if(kind==8)source.prepareFail=true;if(kind==9)source.sealFail=true;if(kind==10)io.openFail=true;if(kind==11)io.closeFail=true;
  {Session s(io,source);bool okay=s.Validate();bool clean=okay && s.Finish();assert(!clean);
   unsigned stopped=io.calls,observed=source.observers;s.Abort();s.Abort();
   assert(!s.Finish() && !s.Validate() && io.calls==stopped && observed==source.observers);
   if(stopped<totalCalls)assert(!io.ends);
  }
  assert(io.closes==unsigned(io.opens && !io.openFail));failures++;
 };
 // EVERY transport call can fail independently. No later IO/END is allowed.
 for(unsigned at=1;at<=totalCalls;at++){check(0,at);if(!(at%64))printf("DEBUG70_CLIENT_FAULT_PROGRESS %u/%u\n",at,totalCalls);}
 for(unsigned at:{1U,5U,6U,totalCalls/2,totalCalls-2,totalCalls-1,totalCalls}){check(1,at);check(2,at);}
 for(unsigned at:{1U,12U,13U,32768U,63266U})check(3,at);
 for(unsigned at=0;at<64;at++)if(at!=9 && at!=13)check(4,at);
 for(unsigned kind=5;kind<=11;kind++)check(kind,0);
 printf("PASS DEBUG70 actual client failure cases%u/ALL%ucall failures/changed source+epoch/partial raw pages/62header corruptions/strict observer failure+drop+differences/open-close-prepare-seal;CLOSE ONLY,no END/reset/retry after uncertainty;NOT original numeric or physical qualification\n",failures,totalCalls);
}
