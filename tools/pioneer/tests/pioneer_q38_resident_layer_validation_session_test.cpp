/* SPDX-License-Identifier: MIT */
// Actual production client lifecycle over SYNTHETIC source/OS replies.
// This test is NOT model authentication,numerical or silicon qualification.
#define PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1 // canonical DATA comparison only
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38ResidentLayerValidationSession.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
struct SessionSource {
 PioneerQwen38Gguf model;FILE* file;
 bool missingReference=false,badReferenceReads=false,policyFailed=false,badReport=false;
 bool valid=true,prepared=false,sealed=false,gpu=false,returned=false,prepareFail=false,sealFail=false,observerFail=false,dropObserver=false;
 unsigned prepares=0,seals=0,reads=0,failRead=0,observers=0;
 explicit SessionSource(FILE* f):file(f){assert(model.Open(f));}
 bool Ready()const{return valid && prepared && model.Stable();}
 bool Prepare(unsigned token){assert(!gpu && !prepares++ && token==0);prepared=!prepareFail;return prepared;}
 bool SealBeforeNativeContext(){assert(!gpu && prepared && !seals++);sealed=!sealFail;return sealed;}
 bool FfnWeightsReady()const{return sealed && Ready();}
 const PioneerQwen38Gguf* SealedModel()const{return sealed && Ready()?&model:nullptr;}
 bool ReadWeightPage(const PioneerQwen38PackedPlan& plan,unsigned part,unsigned page,uint8_t* bytes,unsigned capacity){
  assert(gpu && sealed && prepared && valid && capacity==4096 && part<plan.Parts().size());
  const auto& p=plan.Parts()[part];assert(p.kind==PIONEER_Q38_WEIGHT && page<p.allocated/4096 && plan.MatchesSource(model));
  if(++reads==failRead)return false;
  memset(bytes,0,4096);uint32_t marker[4]={0x53594e54,part,page,~page};memcpy(bytes,marker,sizeof(marker));return true;
 }
 bool ResidentLayerReferenceReady()const{return sealed && Ready() && !missingReference;}
 uint64_t ReferenceReads()const{return badReferenceReads?4490:4810;}
 bool Observe(const uint32_t* values,unsigned words,PioneerQ38ResidentLayerComparison* report,bool policy){
  assert(returned && gpu && sealed && words==1630208 && report);if(!Ready())return false;observers++;
  *report={};report->checked=badReport?1629120:words;report->passed=policy && !policyFailed;report->finalDifferences=policy?0:1;
  // SYNTHETIC sentinel ONLY,not original arithmetic or a hardware result.
  if(dropObserver)valid=false;
  return !observerFail && values[0]==0x54455354 && values[1630207]==0x454e4421;
 }
 bool CompareResidentLayer(const uint32_t* values,unsigned words,PioneerQ38ResidentLayerComparison* report){return Observe(values,words,report,false);}
 bool CompareResidentLayerInstruction(const uint32_t* values,unsigned words,PioneerQ38ResidentLayerComparison* report){return Observe(values,words,report,true);}

};
struct SessionIO {
 SessionSource& source;PioneerQwen38PackedPlan plan;
 struct Part{unsigned index,pages;};std::vector<Part> selected;
 unsigned opens=0,closes=0,calls=0,begins=0,spans=0,spanEnds=0,runs=0,ends=0,uploaded=0,partAt=0,pageAt=0;
 unsigned failCall=0,sourceDrop=0,epochDrop=0,badCall=0,badOffset=0,badHeader=UINT32_MAX;
 uint32_t badXor=1;bool openFail=false,closeFail=false;
 bool(*earlyFinish)(void*)=nullptr;void* client=nullptr;unsigned earlyChecks=0;
 explicit SessionIO(SessionSource& s):source(s){
  assert(plan.Build(source.model,8192,true,true));auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
  assert(canonical->Count()==plan.Parts().size());
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];const auto& c=*canonical->Part(i);
   assert(p.kind==c.kind && p.layer==c.layer && p.role==c.role && p.bank==c.bank && p.firstRow==c.firstRow && p.rows==c.rows
    && p.rowBytes==c.rowBytes && p.proposedAddress==canonical->Base+c.offset && p.bytes==c.bytes && p.allocated==c.allocated);
   if(p.kind!=PIONEER_Q38_WEIGHT)continue;
   if(p.layer==64 && !p.role && !p.firstRow)selected.insert(selected.begin(),{i,12});
   if(!p.layer && p.role<=13)selected.push_back({i,p.allocated/4096});
  }assert(selected.size()==57);
 }
 int Open(){assert(source.sealed && source.prepared && source.Ready() && !source.gpu && !opens++);if(openFail)return -1;source.gpu=true;return 73;}
 int Close(int fd){assert(fd==73 && !closes++);source.gpu=false;return closeFail?-1:0;}
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  assert(fd==73 && !closes && source.Ready() && source.gpu);calls++;
  if(earlyFinish && (op==TT_PCI_PROBE_Q38_BANK_BEGIN || op==TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATE)){
   unsigned before=calls;assert(!earlyFinish(client) && calls==before);earlyChecks++;
  }
  switch(op){
   case TT_PCI_PROBE_RELOAD_STATUS:{auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
    assert(bytes==sizeof(r) && r.version==1);r.driver=55;r.guard_version=1;r.open_count=1;r.generation=4;r.build_tag=UINT64_C(0x2026102600010069);break;}
   case TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATION_SUPPORT:
    assert(bytes==sizeof(tt_q38_embedding_support));*static_cast<tt_q38_embedding_support*>(data)={1,55,20,1630208,248320,512,0,0};break;
   case TT_PCI_PROBE_Q38_BANK_SUPPORT:*static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,PioneerResidentBanks::Capacity};break;
   case TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT:*static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};break;
   case TT_PCI_PROBE_Q38_BANK_BEGIN:{begins++;auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    assert(!source.observers);r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=PioneerResidentBanks::Total;break;}
   case TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD:{auto& r=*static_cast<tt_q38_span_upload*>(data);assert(partAt<selected.size());
    const auto part=selected[partAt];const auto& p=plan.Parts()[part.index];unsigned pages=std::min(256U,part.pages-pageAt);
    assert(bytes==sizeof(r) && begins==1 && !runs && !spanEnds && !source.observers && r.version==1 && !r.reserved && r.bank==p.bank
     && r.offset==p.proposedAddress-plan.Base+uint64_t(pageAt)*4096 && r.bytes==pages*4096 && !r.result.state);
    for(unsigned page=0;page<pages;page++){
     const auto* words=r.data+page*1024;assert(words[0]==0x53594e54 && words[1]==part.index && words[2]==pageAt+page
      && words[3]==~(pageAt+page) && !words[4] && !words[1023]);
    }
    for(unsigned word=pages*1024;word<262144;word++)assert(!r.data[word]);
    spans++;uploaded+=pages;pageAt+=pages;if(pageAt==part.pages){partAt++;pageAt=0;}
    r.result={1,TT_RUNTIME_PASSED,pages*4096,pages,spans,1,1,1,0,0,0,0};break;}
   case TT_PCI_PROBE_Q38_SPAN_END:
    assert(partAt==57 && !pageAt && uploaded==68603 && spans==300 && !runs && !source.observers);spanEnds++;
    *static_cast<tt_q38_span_end*>(data)={1,0,1,0};break;
   case TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATE:{runs++;auto& reply=*static_cast<tt_q38_resident_layer_validation*>(data);
    assert(bytes==sizeof(reply) && PioneerQ38ResidentLayerValidationRequest(reply) && !reply.metadata.token && spanEnds==1 && !source.observers);
    auto& r=reply.metadata;r.state=TT_RUNTIME_PASSED;r.closed=r.input_ready=1;r.session=7;r.driver_generation=4;r.elapsed_us=123;
    r.source_pages=68603;r.workspace_pages=512;r.saved_words=r.restored_words=19282434;
    reply.state_runs=672;reply.state_pages=73728;reply.history_runs=336;reply.history_pages=1680;
    reply.projection_runs=4;reply.projection_boots=119;reply.projection_root_runs=reply.projection_root_holds=5;
    reply.cohort_boots=119;reply.cohort_root_runs=reply.cohort_root_holds=1;reply.cohort_stopped=reply.cohort_held=119;
    reply.convolution_jobs=640;reply.gdn_halves=96;reply.output_chunks=24;reply.output_rows=5120;
    reply.readback_pages=1592;reply.readback_words=1630208;reply.next_slot=1;
    reply.saved_words=reply.restored_words=11000888;reply.verified_words=22001776;reply.installed_words=2270704;
    reply.prefix_saved_words=reply.prefix_restored_words=8134072;reply.prefix_verified_words=16268144;reply.prefix_installed_words=989304;
    reply.ffn_matrices=357;reply.ffn_producers=88;reply.final_rows=5120;
    reply.values[0]=0x54455354;reply.values[1630207]=0x454e4421;
    if(badHeader<64){auto* field=reinterpret_cast<uint8_t*>(&reply)+badHeader*4;uint32_t value;memcpy(&value,field,4);
     if(badHeader==8 || badHeader==12)value=0;else value^=1;memcpy(field,&value,4);}
    source.returned=true;break;}
   case TT_PCI_PROBE_Q38_BANK_END:{ends++;assert(source.observers==2 && runs==1);auto& r=static_cast<tt_q38_bank_session*>(data)->result;
    r.version=1;r.state=TT_RESIDENT_SESSION_CLOSED;r.host_released=r.pci_restored=1;r.bytes=PioneerResidentBanks::Total;break;}
   default:assert(!"Old namespace/Work/numeric input/fallback endpoint MUST NOT be called");
  }
  if(calls==badCall){assert(badOffset+4<=bytes);auto* field=static_cast<uint8_t*>(data)+badOffset;uint32_t value;memcpy(&value,field,4);value^=badXor;memcpy(field,&value,4);}
  if(calls==sourceDrop)source.valid=false;
  if(calls==epochDrop)assert(source.model.Open(source.file));
  return calls!=failCall;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;unsigned positives=0,faults=0;
 using Session=PioneerQ38ResidentLayerValidationSession<SessionIO,SessionSource>;
 {
  SessionSource source(fixture.file);SessionIO io(source);Session s(io,source);io.client=&s;io.earlyFinish=[](void* p){return static_cast<Session*>(p)->Finish();};
  assert(!s.Finish() && !s.Validate(1) && !s.Validate(0,1) && !io.opens);
  assert(s.Validate() && io.calls==307 && source.reads==68603 && io.spans==300 && io.earlyChecks==2
   && source.observers==2 && s.Finish() && io.calls==308 && io.ends==1 && io.closes==1);
  assert(!s.Finish() && !s.Validate() && io.calls==308);positives++;
 }
 puts("PASS actual DEBUG69 client:Prepare+seal BEFORE FD/SAME model epoch/ALL56layer0+embedding68603pages/300spans/307validate+1ENDcalls/earlyEND inert;SYNTHETIC byte+OS endpoints only");
 auto check=[&](unsigned kind,unsigned at){
  SessionSource source(fixture.file);SessionIO io(source);
  if(kind==0)io.failCall=at;
  if(kind==1)io.sourceDrop=at;
  if(kind==2)io.epochDrop=at;
  if(kind==3)source.failRead=at;
  if(kind==4)io.badHeader=at;
  if(kind==5)source.observerFail=true;
  if(kind==6)source.dropObserver=true;
  if(kind==7)io.openFail=true;
  if(kind==8)source.prepareFail=true;
  if(kind==9)source.sealFail=true;
  if(kind==10)io.closeFail=true;
  if(kind==11)source.missingReference=true;if(kind==12)source.badReferenceReads=true;
  if(kind==13)source.policyFailed=true;if(kind==14)source.badReport=true;
  {
   Session s(io,source);bool okay=s.Validate();bool clean=okay && s.Finish();assert(!clean);
   unsigned stopped=io.calls,observed=source.observers;s.Abort();s.Abort();
   assert(!s.Finish() && !s.Validate() && io.calls==stopped && source.observers==observed);
   if(kind!=10 && stopped<308)assert(!io.ends);
  }
  assert(io.closes==unsigned(io.opens && !io.openFail));faults++;
 };
 for(unsigned at=1;at<=308;at++){check(0,at);if(at%64==0)printf("DEBUG69_CLIENT_FAULT_PROGRESS call%u/308\n",at);}
 for(unsigned at:{1U,2U,4U,5U,6U,155U,305U,306U,307U,308U}){check(1,at);check(2,at);}
 for(unsigned at:{1U,12U,13U,256U,32768U,68603U})check(3,at);
 for(unsigned at=0;at<64;at++){
  if(at==9 || at==13)continue; // high half of nonzero unpredictable session/elapsed
  check(4,at);
 }
 for(unsigned kind=5;kind<=14;kind++)check(kind,0);
 printf("PASS actual DEBUG69 client positives%u/faults%u including ALL308call failures/changed source+epoch/missing raw pages/62fixed header corruptions/two distinct observers/policy rejection/old-readback-report refusal/open+close/preparation/seal/ref-ready failures;CLOSE ONLY/no END-read-reset-retry after failure;NOT numerical or silicon qualification\n",positives,faults);
}
