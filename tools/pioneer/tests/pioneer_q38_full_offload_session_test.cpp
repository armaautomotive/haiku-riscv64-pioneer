/* SPDX-License-Identifier: MIT */
// Actual dynamic USER lifecycle over synthetic file/pages/transport ONLY.
// There is no native engine,learned model product or physical card here.
#define PIONEER_Q38_FULL_OFFLOAD_CLIENT 1
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38FullOffloadSession.h"
#include <sys/wait.h>
struct Sweep {
 bool child=false,enabled=true;unsigned cases=0,fence=0;const unsigned* calls=nullptr;
 template<class Change>bool Clone(Change change){
  if(!enabled)return false;
  assert(!child && calls);fflush(nullptr);const auto pid=fork();assert(pid>=0);
  if(!pid){child=true;fence=*calls;change();return true;}
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;return false;
 }
};
struct Source {
 Fixture& file;PioneerQwen38Gguf model;PioneerQwen38PackedPlan plan;
 bool valid=true,opened=false;unsigned opens=0;uint64_t pages=0,bytes=0,reads=0;
 explicit Source(Fixture& f):file(f){}
 bool Open(){assert(!opens++);opened=model.Open(file.file) && plan.Build(model,8192,true,true);
  if(opened)for(const auto& p:plan.Parts())if(p.kind==PIONEER_Q38_WEIGHT){pages+=p.allocated/4096;bytes+=p.allocated;}
  return opened;}
 bool Ready()const{return opened && valid && model.Stable();}
 const auto& Model()const{return model;}
 bool ReadWeightPage(const PioneerQwen38PackedPlan&,unsigned f,unsigned page,uint8_t* output,unsigned capacity){
  const auto& part=plan.Parts().at(f);assert(Ready() && part.kind==PIONEER_Q38_WEIGHT && page<part.allocated/4096 && capacity==4096);
  memset(output,int((f+page)%251+1),4096);reads++;return true;
 }
};
struct IO {
 Source& source;Sweep& sweep;
 unsigned opens=0,closes=0,calls=0,begins=0,uploads=0,spanEnds=0,starts=0,polls=0,cancels=0,holds=0,reads=0,ends=0;
 unsigned prompts=0,limit=0,eos0=0,eos1=0,fragment=0,page=0,cursor=0;
 unsigned stopRecords=0,stopStage=1,stopLayers=1,earlyEosToken=248044;bool earlyEos=false;
 uint32_t ids[96]={};uint64_t uploaded=0;
 IO(Source& s,Sweep& w):source(s),sweep(w){sweep.calls=&calls;}
 unsigned Records()const{return cancels?stopRecords:earlyEos?prompts:limit;}
 unsigned Token(unsigned pos)const{return earlyEos && pos+1==Records()?earlyEosToken:1000+pos;}
 int Open(){assert(source.Ready() && opens==closes);opens++;return 88;}
 int Close(int fd){assert(fd==88 && opens==closes+1);closes++;return 0;}
 void Progress(uint64_t done,uint64_t total){assert(done==uploaded && total==source.bytes);}
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==88 && opens==closes+1 && source.Ready());calls++;
  if(op==TT_PCI_PROBE_RELOAD_STATUS){
   auto& r=*static_cast<tt_pioneer_driver_reload*>(data);assert(size==sizeof(r) && r.version==1);
   r.driver=55;r.guard_version=1;r.build_tag=PioneerQ38FullOffload::BuildTag;r.generation=9;r.open_count=1;r.active_owner=begins;
   if(sweep.Clone([&]{r.build_tag=0x2026103100010075ULL;}))return true;
   if(begins && sweep.Clone([&]{r.generation++;}))return true;
  }else if(op==TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT){
   namespace F=PioneerQ38FullOffload;auto& r=*static_cast<F::Support*>(data);assert(size==sizeof(r) && F::SupportRequest(r));
   r={55,1,96,32,128,248320,F::AllCompute,0};
   for(unsigned word=0;word<8;word++)if(sweep.Clone([&]{reinterpret_cast<uint32_t*>(&r)[word]^=1;}))return true;
  }else if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT){
   *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,UINT64_C(3)*1024*1024*1024};
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT){
   *static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};
  }else if(op==TT_PCI_PROBE_Q38_BANK_BEGIN){
   assert(!begins++);auto& r=static_cast<tt_q38_bank_session*>(data)->result;
   r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=UINT64_C(21)*1024*1024*1024;
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD){
   assert(begins==1 && !spanEnds && !starts);auto& r=*static_cast<tt_q38_span_upload*>(data);
   const auto& parts=source.plan.Parts();while(fragment<parts.size() && parts[fragment].kind!=PIONEER_Q38_WEIGHT)fragment++;
   const auto& p=parts.at(fragment);const unsigned count=std::min(256U,p.allocated/4096-page);
   assert(size==sizeof(r) && r.version==1 && !r.reserved && r.bank==p.bank && r.bytes==count*4096
    && r.offset==p.proposedAddress-0x04000000U+uint64_t(page)*4096);
   for(unsigned n=0;n<count;n++){const auto* raw=reinterpret_cast<const uint8_t*>(r.data+n*1024);
    assert(raw[0]==(fragment+page+n)%251+1 && raw[4095]==raw[0]);}
   uploads++;uploaded+=r.bytes;page+=count;if(page==p.allocated/4096){fragment++;page=0;}
   r.result={1,TT_RUNTIME_PASSED,count*4096,count,uploads,1,1,1,0,0,0,0};
   if(uploads==1 && sweep.Clone([&]{r.result.nonce++;}))return true;
  }else if(op==TT_PCI_PROBE_Q38_SPAN_END){
   assert(uploaded==source.bytes && source.reads==source.pages && !spanEnds++ && !starts);
   *static_cast<tt_q38_span_end*>(data)={1,0,1,0};
  }else if(op==TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL){
   namespace P=PioneerQ38TokenControl;auto& p=*static_cast<P::Packet*>(data);
   assert(size==sizeof(p) && spanEnds==1 && PioneerQ38FullOffload::RequestShape(p));
   auto& r=p.reply;const unsigned action=p.request.operation;r.version=1;r.operation=action;r.started=r.ready=1;
   auto* d=r.detail;d[0]=d[1]=1;
   if(action==P::Start){
    assert(!starts++);prompts=p.request.prompts;limit=p.request.limit;eos0=p.request.eos0;eos1=p.request.eos1;
    std::copy(p.prompt,p.prompt+prompts,ids);r.stage=d[2]=8;d[3]=1;
   }else if(action==P::Observe){
    assert(starts==1 && !holds);polls++;
    if(polls==1){r.ready=0;r.stage=d[2]=9;}
    else{r.stage=d[2]=11;d[6]=1;d[7]=cancels!=0;d[8]=Records();
     d[9]=cancels?stopRecords-unsigned(stopStage==3):Records()-1;d[10]=cancels?stopLayers:64;d[11]=cancels?stopStage:0;
     for(unsigned word:{0U,1U,2U,3U,5U,6U,7U,8U,9U,10U,11U,14U,15U})
      if(sweep.Clone([&]{d[word]^=word==8?256U:word==9?256U:1U;}))return true;
    }
   }else if(action==P::Cancel){assert(!cancels++ && !holds);r.stage=d[2]=7;d[9]=1;}
   else if(action==P::Hold){assert(polls==2 && !holds++ && !reads);r.stage=d[2]=4;d[6]=1;d[7]=120;}
   else if(action==P::Read){
    const unsigned count=std::min(64U,Records()-cursor);assert(holds==1 && p.request.records==count && !ends);
    reads++;r.stage=d[2]=4;r.records=count;d[6]=1;d[7]=cursor;d[8]=count;d[9]=count!=0;d[10]=count*256;d[11]=cursor+count==Records();
    for(unsigned n=0;n<count;n++){
     const unsigned pos=cursor+n;auto* v=p.records+n*64;v[0]=0x31435451;v[1]=1;v[2]=pos;
     v[3]=pos<prompts?ids[pos]:Token(pos-1);v[4]=Token(pos);v[5]=0x3f800000;v[6]=pos+1;v[7]=(pos+1)&1;
     v[8]=pos+1<prompts;v[9]=pos+1!=limit && !(pos+1>=prompts && PioneerQ38FullOffloadEog::Contains(v[4]));v[10]=64;v[11]=973;
     for(unsigned i=12;i<36;i++)v[i]=100+i;v[44]=pos+1;v[45]=48;v[46]=119;
    }
    for(unsigned word:{0U,3U,4U,5U,8U,9U,10U,11U,12U,38U,44U,45U,46U,47U})
     if(count && sweep.Clone([&]{p.records[word]^=word==4?0x80000000U:word==5?0x40000000U:1U;}))return true;
    for(unsigned word:{7U,8U,9U,10U,11U,14U,15U})if(sweep.Clone([&]{d[word]^=1;}))return true;
    cursor+=count;
   }else if(action==P::Finish){assert(cursor==Records() && reads && !ends++);r.stage=d[2]=11;r.complete=1;d[3]=d[5]=1;}
   else assert(!"no hybrid matrix or CPU fallback operation");
   for(unsigned word:{0U,1U,3U,4U,5U,7U,8U,9U})
    if(sweep.Clone([&]{reinterpret_cast<uint32_t*>(&r)[word]^=1;}))return true;
   if(sweep.Clone([&]{p.request.operation^=1;}))return true;
   if(sweep.Clone([&]{p.prompt[8000]=1;}))return true;
   if(action!=P::Read && sweep.Clone([&]{p.records[4095]=1;}))return true;
  }else assert(!"unrecognized operation / no CPU learned operations");
  if(op!=TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || uploads==1){
   if(sweep.Clone([]{}))return false;
   if(sweep.Clone([&]{source.valid=false;}))return true;
  }
  return true;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;Source source(fixture);Sweep sweep;IO io(source,sweep);
 auto session=std::make_unique<PioneerQ38FullOffloadSession<IO,Source>>(io,source);
 uint32_t prompt[96];for(unsigned i=0;i<96;i++)prompt[i]=i+17;
 uint32_t answer[32];std::fill(answer,answer+32,0xaaaaaaaaU);unsigned count=123;
 assert(session->KnownClean());
 assert(!session->Poll() && !session->Finish() && !session->Cancel() && !session->CopyGeneratedTokens(answer,32,count));
 assert(!session->Begin(nullptr,1,1,2,3) && !session->Begin(prompt,97,1,2,3) && !session->Begin(prompt,1,33,2,3) && !io.opens);
 bool okay=session->Probe();
 if(okay){assert(io.opens==1 && io.closes==1 && !io.begins && !io.starts);okay=session->Begin(prompt,96,32,248044,248046);}
 if(okay)assert(!session->KnownClean());
 if(okay)for(unsigned variant=0;variant<9;variant++)if(sweep.Clone([&]{sweep.enabled=false;})){
  const unsigned eos[]={248044,248046,248063,248064,248065};
  io.earlyEos=variant>=1 && variant<=5;if(io.earlyEos)io.earlyEosToken=eos[variant-1];
  if(variant>=6){io.stopRecords=variant==6?0:variant==7?96:97;io.stopStage=variant==6?1:3;io.stopLayers=variant==6?1:64;assert(session->Cancel());}
  assert(session->Poll() && !session->TerminalReady() && !session->Finish());
  assert(session->Poll() && session->TerminalReady() && !session->CopyGeneratedTokens(answer,32,count));
  assert(session->Finish() && session->Completed() && session->KnownClean() && io.ends==1 && io.closes==2);
  assert(session->CopyGeneratedTokens(answer,32,count));
  assert(count==(variant==0?32U:variant<=6?0U:variant==7?1U:2U));
  for(unsigned i=0;i<count;i++)assert(answer[i]==1095+i);
  for(unsigned i=count;i<32;i++)assert(answer[i]==0xaaaaaaaaU);
  const auto calls=io.calls;assert(!session->Begin(prompt,1,1,2,3) && !session->Poll() && !session->Finish() && !session->Cancel());
  session.reset();assert(io.calls==calls);_exit(0);
 }
 if(okay){okay=session->Poll();if(okay)okay=session->Poll();if(okay)okay=session->Finish();}
 if(sweep.child){
  assert(!okay && session->FailedState() && io.calls==sweep.fence && io.closes==io.opens);
  assert(count==123);for(auto word:answer)assert(word==0xaaaaaaaaU);
  const auto calls=io.calls;session->Abort();session.reset();assert(calls==io.calls);_exit(0);
 }
 assert(okay && session->CopyGeneratedTokens(answer,32,count) && count==32 && io.reads==2 && io.ends==1);
 assert(source.opens==1 && io.opens==2 && io.closes==2 && source.reads==source.pages);
 printf("PASS actual full-offload USER:96prompt/32output/127positions,two64-record drains,all5 EOS and3 cancellation boundaries,%u fail-closed COW fixtures;synthetic ONLY,no physical or numerical result\n",sweep.cases);
}
