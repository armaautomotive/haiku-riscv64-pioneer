/* SPDX-License-Identifier: MIT */
// Synthetic source/OS/transport only, not a physical or model-speed result.
#define PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT 1
#define PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT 1
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38TokenHiddenValidationSession.h"
#include "../PioneerQ38DecodeBenchmarkCli.h"
#ifdef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT
#include "../PioneerQ38FrontierBenchmarkCli.h"
#endif
#include <sys/wait.h>
namespace P=PioneerQ38TokenControl;
namespace V=PioneerQ38TokenDecodeBenchmark;
static unsigned protocolCases=0;
static void Protocol(){
#ifdef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT
 using F=PioneerQ38FrontierBenchmarkMode;
 const char* fresh[]={"benchmark","--benchmark-34-positions",nullptr};
 assert(PioneerQ38FrontierBenchmarkArguments(2,fresh)==F::Positions34);
 for(const char* text:{"--generate-34-positions","--generate-three-positions","--work","--cpu","--benchmark-34-positions=1",""}){
  const char* bad[]={"benchmark",text,nullptr};assert(PioneerQ38FrontierBenchmarkArguments(2,bad)==F::Invalid);protocolCases++;
 }
 assert(TT_PCI_PROBE_Q38_TOKEN_FRONTIER_BENCHMARK!=TT_PCI_PROBE_Q38_TOKEN_DECODE_BENCHMARK
  && TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE!=TT_PCI_PROBE_Q38_TOKEN_FRONTIER_BENCHMARK);
#endif
 using Mode=PioneerQ38DecodeBenchmarkMode;
 const char* run[]={"benchmark","--generate-34-positions",nullptr};
 const char* source[]={"benchmark","--preflight-source-only",nullptr};
 assert(PioneerQ38DecodeBenchmarkArguments(2,run)==Mode::Positions34
  && PioneerQ38DecodeBenchmarkArguments(2,source)==Mode::SourceOnly);
 for(int count:{-1,0,1,3,4,100}){assert(PioneerQ38DecodeBenchmarkArguments(count,run)==Mode::Invalid);protocolCases++;}
 assert(PioneerQ38DecodeBenchmarkArguments(2,nullptr)==Mode::Invalid);protocolCases++;
 for(const char* text:{"","--generate-three-positions","--generate-34-positions=1","--generate-34-positions ",
                      " --generate-34-positions","--generate-35-positions","--work","--cpu","--help",
                      "--capture-first-layer","--preflight-source-only=1"}){
  const char* bad[]={"benchmark",text,nullptr};
  assert(PioneerQ38DecodeBenchmarkArguments(2,bad)==Mode::Invalid);protocolCases++;
 }
 auto p=std::make_unique<P::Packet>();
 auto start=[&](){*p={};p->request.magic=P::Magic;p->request.version=P::Version;p->request.operation=P::Start;
  p->request.prompts=V::Prompts;p->request.limit=V::Positions;p->prompt[0]=V::FirstToken;p->prompt[1]=V::SecondToken;};
 start();assert(V::ControlRequestShape(*p) && !PioneerQ38TokenHiddenValidation::ControlRequestShape(*p));
 assert(TT_PCI_PROBE_Q38_TOKEN_DECODE_BENCHMARK!=TT_PCI_PROBE_Q38_TOKEN_HIDDEN_CONTROL
  && TT_PCI_PROBE_Q38_TOKEN_DECODE_BENCHMARK!=TT_PCI_PROBE_Q38_TOKEN_HIDDEN_READ);
 // Every bit of the fixed Start header and prompt must stay exact.
 for(unsigned word=0;word<18;word++)for(unsigned bit=0;bit<32;bit++){
  start();reinterpret_cast<uint32_t*>(p.get())[word]^=1U<<bit;
  assert(!V::ControlRequestShape(*p));protocolCases++;
 }
 for(unsigned word=18;word<sizeof(P::Packet)/4;word++){
  start();reinterpret_cast<uint32_t*>(p.get())[word]=1;
  assert(!V::ControlRequestShape(*p));protocolCases++;
 }
 for(unsigned limit:{0U,1U,2U,3U,33U,35U,64U,8192U,UINT32_MAX}){
  start();p->request.limit=limit;assert(!V::ControlRequestShape(*p));protocolCases++;
 }
 for(unsigned operation=P::Start;operation<=P::Finish;operation++){
  *p={};auto& r=p->request;r.magic=P::Magic;r.version=P::Version;r.operation=operation;
  if(operation==P::Read)r.records=V::Positions;
  assert(V::ControlRequestShape(*p)==bool(operation!=P::Start && operation!=P::Cancel));
 }
 *p={};p->request.magic=P::Magic;p->request.version=P::Version;p->request.operation=P::Read;
 for(unsigned count:{0U,1U,3U,33U,35U,64U,UINT32_MAX}){
  p->request.records=count;assert(!V::ControlRequestShape(*p));protocolCases++;
 }
}
struct Source {
 Fixture& fixture;PioneerQwen38Gguf model;PioneerQwen38PackedPlan expected;
 bool current=true;unsigned opens=0;uint64_t pages=0;
 explicit Source(Fixture& f):fixture(f){}
 bool Open(){assert(!opens++);return model.Open(fixture.file) && expected.Build(model,8192,true,true);}
 bool Ready()const{return current && opens==1 && model.Stable();}
 const auto& Model()const{return model;}
 bool ReadWeightPage(const PioneerQwen38PackedPlan&,unsigned fragment,unsigned page,uint8_t* out,unsigned bytes){
  assert(Ready() && bytes==4096);const auto& part=expected.Parts().at(fragment);
  assert(part.kind==PIONEER_Q38_WEIGHT && page<part.allocated/4096);pages++;
  std::memset(out,int((fragment+page)%255+1),4096);return true;
 }
};
struct IO {
 Source& source;unsigned opens=0,closes=0,calls=0,begins=0,uploads=0,spanEnds=0,starts=0,polls=0,holds=0,reads=0,ends=0;
 uint64_t bytes=0;unsigned fragment=0,page=0;
 // Inject only synthetic reply damage after the expensive MOCK uploader.
 unsigned damagePhase=0,damageWord=0,damageRecord=0;uint32_t damageMask=1;bool failCopyClose=false;
#ifdef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT
 unsigned metrics=0,metricDamage=0;
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 uint32_t firstRaw=0x60008,secondRaw=0x40004;
#endif
 explicit IO(Source& s):source(s){}
 int Open(){assert(source.Ready() && !opens++);return 76;}
 int Close(int fd){assert(fd==76 && !closes++);return failCopyClose?-1:0;}
 void Progress(uint64_t done,uint64_t total){assert(done==bytes && done<=total);}
 bool Call(int fd,unsigned op,void* data,size_t length){
  assert(fd==76 && opens==1 && !closes && source.Ready());calls++;
  if(op==TT_PCI_PROBE_RELOAD_STATUS){
   assert(length==sizeof(tt_pioneer_driver_reload));auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
   r.driver=55;r.guard_version=1;r.build_tag=V::BuildTag;r.generation=7;r.open_count=1;r.active_owner=begins;
  }else if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT){
   *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,UINT64_C(3)*1024*1024*1024};
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT){
   *static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};
  }else if(op==TT_PCI_PROBE_Q38_BANK_BEGIN){
   assert(!begins++);auto& r=static_cast<tt_q38_bank_session*>(data)->result;
   r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=UINT64_C(21)*1024*1024*1024;
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD){
   assert(length==sizeof(tt_q38_span_upload) && begins==1 && !spanEnds && !starts);
   auto& r=*static_cast<tt_q38_span_upload*>(data);const auto& parts=source.expected.Parts();
   while(fragment<parts.size() && parts[fragment].kind!=PIONEER_Q38_WEIGHT)fragment++;
   const auto& part=parts.at(fragment);const unsigned count=std::min(256U,part.allocated/4096-page);
   assert(r.version==1 && !r.reserved && r.bank==part.bank && r.bytes==count*4096
    && r.offset==part.proposedAddress-0x04000000U+uint64_t(page)*4096);
   for(unsigned i=0;i<count;i++){
    const auto* raw=reinterpret_cast<const uint8_t*>(r.data+i*1024);const auto value=(fragment+page+i)%255+1;
    assert(raw[0]==value && raw[4095]==value);
   }
   uploads++;bytes+=r.bytes;page+=count;if(page==part.allocated/4096){fragment++;page=0;}
   r.result={1,TT_RUNTIME_PASSED,count*4096,count,uploads,1,1,1,0,0,0,0};
  }else if(op==TT_PCI_PROBE_Q38_SPAN_END){
   assert(bytes==UINT64_C(19399024640) && !spanEnds++);*static_cast<tt_q38_span_end*>(data)={1,0,1,0};
#ifdef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT
  }else if(op==TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE){
   namespace F=PioneerQ38TokenFrontierControl;
   assert(length==sizeof(F::Packet) && starts==1 && !holds && !ends);auto& p=*static_cast<F::Packet*>(data);
   assert(F::RequestShape(p));metrics++;auto& r=p.reply;
   r.version=1;r.operation=F::Observe;r.started=r.ready=1;
   r.detail[0]=r.detail[1]=r.detail[6]=1;r.detail[2]=12;r.detail[4]=63;
   r.detail[7]=polls>=2?V::Positions:polls?2U:0U;r.detail[8]=V::Positions;r.detail[9]=V::Prompts;
   if(metricDamage==1)r.detail[7]=V::Positions+1;
   if(metricDamage==2)r.detail[8]++;
   if(metricDamage==3)r.detail[9]++;
   if(metricDamage==4)r.detail[10]=1;
   if(metricDamage==5)r.ready=0;
   if(metricDamage==6)r.detail[7]=1; // genuine only as SYNTHETIC reply damage
   if(metricDamage==7)r.version++;
   if(metricDamage==8)r.status=-1;
  }else if(op==TT_PCI_PROBE_Q38_TOKEN_FRONTIER_BENCHMARK){
#else
  }else if(op==TT_PCI_PROBE_Q38_TOKEN_DECODE_BENCHMARK){
#endif
   assert(length==sizeof(P::Packet) && spanEnds==1);auto& p=*static_cast<P::Packet*>(data);
   assert(V::ControlRequestShape(p));const unsigned operation=p.request.operation;
   auto& r=p.reply;r.version=1;r.operation=operation;r.started=r.ready=1;
   auto* d=r.detail;d[0]=d[1]=1;
   if(operation==P::Start){assert(!starts++);r.stage=d[2]=8;d[3]=1;}
   else if(operation==P::Observe){
    assert(starts==1 && !holds && !ends);polls++;
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
    d[12]=firstRaw;d[13]=secondRaw;
#endif
    if(polls==1){r.ready=0;r.stage=d[2]=9;d[4]=63;}
    else{r.stage=d[2]=11;d[6]=1;d[8]=V::Positions;d[9]=V::Positions-1;d[10]=64;
     if(damagePhase==1)d[damageWord]^=damageMask;}
   }else if(operation==P::Hold){assert(polls==2 && !holds++ && !reads && !ends);r.stage=d[2]=4;d[6]=1;d[7]=120;}
   else if(operation==P::Read){
    assert(holds==1 && !reads++ && !ends && p.request.records==V::Positions);
    r.stage=d[2]=4;r.records=V::Positions;d[6]=1;d[8]=V::Positions;d[9]=1;d[10]=V::Positions*256;d[11]=1;
    for(unsigned pos=0;pos<V::Positions;pos++){
     auto* record=p.records+pos*64;
     const unsigned h[]={0x31435451,1,pos,pos==0?248319U:pos==1?17U:99+pos,100+pos,0x3f800000,
      pos+1,(pos+1)&1,unsigned(pos==0),unsigned(pos<V::Positions-1),64,973};
     std::copy(h,h+12,record);for(unsigned i=12;i<36;i++)record[i]=i+100;
     record[44]=pos+1;record[45]=48;record[46]=119;
    }
    if(damagePhase==2)p.records[damageRecord*64+damageWord]^=damageMask;
    if(damagePhase==3)p.records[V::Positions*64]=1;
   }else if(operation==P::Finish){assert(reads==1 && !ends++);r.stage=d[2]=11;r.complete=1;d[3]=d[5]=1;}
   else assert(!"no cancel,hidden capture,old compute,CPU or Work operation");
  }else assert(!"new diagnostic never uses the old short endpoint or hidden reads");
  return true;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Protocol();Fixture fixture;Source source(fixture);IO io(source);
 auto session=std::make_unique<PioneerQ38TokenHiddenValidationSession<IO,Source>>(io,source);
#ifdef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT
 PioneerQ38TokenFrontierControl::Packet beforeOpen;
 assert(!session->ObserveFrontier(beforeOpen) && !io.calls && !io.opens);
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 {PioneerQ38TokenFrontendSnapshot raw;assert(!session->LastPollFrontends(raw) && !io.calls && !io.opens);}
 unsigned frontendCases=0;auto frontends=std::make_unique<PioneerQ38TokenFrontendSamples>();
#endif
 std::array<uint32_t,V::Positions*64> output;output.fill(0xaaaaaaaaU);
 auto unchanged=[&](){for(auto word:output)assert(word==0xaaaaaaaaU);};
 assert(!session->CopyGenerationRecords(output.data(),output.size()));unchanged();
 assert(session->Begin() && io.uploads==20367 && source.pages==4736090 && io.bytes==UINT64_C(19399024640));
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 {PioneerQ38TokenFrontendSnapshot raw;const auto calls=io.calls;assert(!session->LastPollFrontends(raw) && io.calls==calls);}
#endif
 unsigned branches=0;
 auto child=[&](auto check){
  fflush(nullptr);pid_t pid=fork();assert(pid>=0);if(!pid){check();_exit(0);}
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));branches++;
 };
 auto finish=[&](){
#ifdef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT
  if(session->TerminalReady()){
   PioneerQ38TokenFrontierControl::Packet metric;assert(session->ObserveFrontier(metric));
  }
#endif
  return session->Finish();
 };
 auto fail=[&](bool expectedTerminal){
  assert(session->Poll() && !session->TerminalReady() && !session->Finish());
  const bool ready=session->Poll();assert(ready==expectedTerminal);
  if(ready)assert(!finish());
  assert(io.closes==1 && !session->Completed() && !io.ends);
  assert(!session->CopyGenerationRecords(output.data(),output.size()));unchanged();
  const auto calls=io.calls;session.reset();assert(io.calls==calls && io.closes==1);
 };
 for(unsigned word:{0U,1U,2U,3U,5U,6U,7U,8U,9U,10U,11U,14U,15U})child([&]{io.damagePhase=1;io.damageWord=word;fail(false);});
 for(unsigned pos=0;pos<V::Positions;pos++)for(unsigned word:{0U,1U,2U,3U,6U,7U,8U,9U,10U,11U,12U,35U,38U,44U,45U,46U,47U,63U})
  child([&]{io.damagePhase=2;io.damageRecord=pos;io.damageWord=word;fail(true);});
 for(unsigned pos=0;pos<V::Positions;pos++){
  child([&]{io.damagePhase=2;io.damageRecord=pos;io.damageWord=4;io.damageMask=(100+pos)^248320U;fail(true);});
  child([&]{io.damagePhase=2;io.damageRecord=pos;io.damageWord=5;io.damageMask=0x3f800000U^0x7fc00000U;fail(true);});
 }
 child([&]{io.damagePhase=3;fail(true);});
 child([&]{const auto calls=io.calls;assert(!session->RequestCancellation() && io.calls==calls && io.closes==1);});
 child([&]{io.failCopyClose=true;assert(session->Poll() && session->Poll() && !finish() && !session->Completed());});
#ifdef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT
 for(unsigned damage=1;damage<=8;damage++)child([&]{
  assert(session->Poll());PioneerQ38TokenFrontierControl::Packet metric;assert(session->ObserveFrontier(metric));
  io.metricDamage=damage;assert(!session->ObserveFrontier(metric) && io.closes==1 && !io.ends && !io.holds);
  const auto calls=io.calls;assert(!session->ObserveFrontier(metric) && !session->Finish() && io.calls==calls);
 });
 child([&]{assert(session->Poll() && session->Poll() && !session->Finish() && io.closes==1 && !io.holds && !io.ends);});
#endif
 assert(session->Poll() && !session->TerminalReady() && !session->CopyGenerationRecords(output.data(),output.size()));unchanged();
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 {
  const auto calls=io.calls;PioneerQ38TokenFrontendSnapshot raw;
  assert(session->LastPollFrontends(raw) && io.calls==calls && raw.generation==7 && !raw.ready && raw.stage==9
   && raw.checkedWord==63 && raw.firstPC==io.firstRaw && raw.secondPC==io.secondRaw);
  assert(frontends->Observe(100,110,0,raw));frontendCases++;
  for(unsigned field=0;field<7;field++){
   PioneerQ38TokenFrontendSnapshot dirty;
   if(field==0)dirty.generation=1;if(field==1)dirty.ready=1;if(field==2)dirty.stage=1;
   if(field==3)dirty.checkedWord=1;if(field==4)dirty.firstPC=1;if(field==5)dirty.secondPC=1;if(field==6)dirty.reserved=1;
   const auto before=dirty;assert(!session->LastPollFrontends(dirty) && !std::memcmp(&before,&dirty,sizeof(dirty)) && io.calls==calls);frontendCases++;
  }
  child([&]{source.current=false;PioneerQ38TokenFrontendSnapshot value;const auto count=io.calls;
   assert(!session->LastPollFrontends(value) && io.calls==count);});
  for(uint32_t pc:{0U,0x40000U,0x60004U,0x60008U,0x64000U,0xfffffffcU,0xffffffffU})child([&]{
   io.firstRaw=pc;io.secondRaw=pc^4U;assert(session->Poll() && session->TerminalReady());
   const auto count=io.calls;PioneerQ38TokenFrontendSnapshot value;
   assert(session->LastPollFrontends(value) && io.calls==count && value.ready && value.stage==11
    && value.firstPC==pc && value.secondPC==(pc^4U));assert(finish() && session->Completed());
  });
 }
#endif
 assert(session->Poll() && session->TerminalReady() && !session->CopyGenerationRecords(output.data(),output.size()));unchanged();
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 {const auto calls=io.calls;PioneerQ38TokenFrontendSnapshot raw;
  assert(session->LastPollFrontends(raw) && io.calls==calls && raw.ready && raw.stage==11);
  assert(frontends->Observe(200,210,2,raw) && frontends->Complete());frontendCases++;}
#endif
 assert(finish() && session->Completed() && session->CommittedRecords()==V::Positions && io.closes==1 && io.ends==1);
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 {const auto calls=io.calls;PioneerQ38TokenFrontendSnapshot raw;
  assert(!session->LastPollFrontends(raw) && io.calls==calls);frontendCases++;}
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT
 {PioneerQ38TokenFrontierControl::Packet metric;const auto calls=io.calls;
  assert(!session->ObserveFrontier(metric) && io.calls==calls && io.metrics==1);}
#endif
 assert(!session->CopyGenerationRecords(nullptr,output.size()) && !session->CopyGenerationRecords(output.data(),192));unchanged();
 assert(session->CopyGenerationRecords(output.data(),output.size()));
 for(unsigned pos=0;pos<V::Positions;pos++)assert(output[pos*64+2]==pos && output[pos*64+4]==100+pos);
 output.fill(0xaaaaaaaaU);source.current=false;
 assert(!session->CopyGenerationRecords(output.data(),output.size()));unchanged();
 const auto calls=io.calls;session.reset();assert(io.calls==calls && io.closes==1);
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 const char* cowLabel="COW branches including frontend positives";
#else
 const char* cowLabel="COW failures";
#endif
 printf("PASS separate34-position/32-feedback MOCK USER lifecycle/full851 synthetic uploader/4736090pages/20367batches/all34 chained records/HOLD+one drain+full END+Close/%u %s and%u protocol refusals;no old endpoint,cancel,hidden capture,CPU model or Work;NO native/model numerics/physical/steady-TPS qualification\n",branches,cowLabel,protocolCases);
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 printf("PASS frontend snapshot getter/%u direct cases+8 COW source/raw-address cases/IO call count UNCHANGED/pending+terminal snapshots/bad output unchanged/closed andwrong-phase refused/raw addresses NOT retiredPC,layer count,cycles or completion authority;MOCK transport ONLY\n",frontendCases);
#endif
}
