/* SPDX-License-Identifier: MIT */
// Actual USER uploader/lifecycle over a SYNTHETIC sparse GGUF fixture,
// synthetic page bytes and OS/driver replies. No native engine/card exists.
// COW fault children share only MOCK progress,never accelerator execution.
#define PIONEER_Q38_TOKEN_VALIDATION_CLIENT 1
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38TokenValidationSession.h"
#include <sys/wait.h>
struct Sweep {
 bool child=false,enabled=true;unsigned clones=0,fence=0;const unsigned* calls=nullptr;
 template<class Change>bool Clone(Change change){
  if(!enabled)return false;
  assert(!child && calls);fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){child=true;fence=*calls;change();return true;}
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));clones++;return false;
 }
};
struct SyntheticSource {
 Fixture& file;Sweep& sweep;PioneerQwen38Gguf model;PioneerQwen38PackedPlan expected;
 bool valid=true,opened=false,openFail=false,readFail=false;unsigned opens=0;uint64_t reads=0,pages=0,bytes=0;
 SyntheticSource(Fixture& f,Sweep& s):file(f),sweep(s){}
 bool Open(){
  assert(!opens++);if(openFail)return false;
  opened=model.Open(file.file) && expected.Build(model,8192,true,true);
  if(opened)for(const auto& p:expected.Parts())if(p.kind==PIONEER_Q38_WEIGHT){pages+=p.allocated/4096;bytes+=p.allocated;}
  return opened;
 }
 bool Ready()const{return valid && opened && model.Stable();}
 const auto& Model()const{return model;}
 bool ReadWeightPage(const PioneerQwen38PackedPlan&,unsigned f,unsigned page,uint8_t* out,unsigned capacity){
  assert(opened && valid && capacity==4096);const auto& p=expected.Parts().at(f);
  assert(p.kind==PIONEER_Q38_WEIGHT && page<p.allocated/4096);reads++;
  if(readFail)return false;
  if((reads==1 || reads==pages) && sweep.Clone([]{}))return false;
  // Deliberately synthetic bytes,NOT original weights or learned products.
  std::memset(out,int((f+page)%255+1),4096);return true;
 }
};
struct SyntheticIO {
 SyntheticSource& source;Sweep& sweep;
 unsigned opens=0,closes=0,calls=0,begins=0,uploads=0,spanEnds=0,starts=0,polls=0,holds=0,reads=0,ends=0;
 unsigned fragment=0,page=0;uint64_t bytes=0,progress=0;bool openFail=false;
 explicit SyntheticIO(SyntheticSource& s):source(s),sweep(s.sweep){sweep.calls=&calls;}
 int Open(){assert(source.Ready() && !opens++);return openFail?-1:74;}
 int Close(int fd){assert(fd==74 && !closes++);if(!sweep.child && ends==1 && sweep.Clone([]{}))return -1;return 0;}
 void Progress(uint64_t done,uint64_t total){assert(done==bytes && done>progress && total==source.bytes);progress=done;}
 bool Call(int fd,unsigned op,void* data,size_t size){
  assert(fd==74 && opens==1 && !closes && source.Ready());calls++;
  if(op==TT_PCI_PROBE_RELOAD_STATUS){
   assert(size==sizeof(tt_pioneer_driver_reload));auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
   assert(r.version==1);r.driver=55;r.guard_version=1;r.build_tag=PioneerQ38TokenValidation::BuildTag;r.generation=4;r.open_count=1;r.active_owner=begins;
   if(sweep.Clone([&]{r.build_tag=0x2026102100010012ULL;}))return true;
   if(begins && sweep.Clone([&]{r.generation++;}))return true;
  }else if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT){
   assert(size==sizeof(tt_q38_bank_support));*static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,UINT64_C(3)*1024*1024*1024};
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT){
   assert(size==sizeof(tt_q38_span_support));*static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};
  }else if(op==TT_PCI_PROBE_Q38_BANK_BEGIN){
   assert(size==sizeof(tt_q38_bank_session) && !begins++);auto& r=static_cast<tt_q38_bank_session*>(data)->result;
   r.version=1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=UINT64_C(21)*1024*1024*1024;
  }else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD){
   assert(size==sizeof(tt_q38_span_upload) && begins==1 && !spanEnds && !starts);
   auto& r=*static_cast<tt_q38_span_upload*>(data);const auto& parts=source.expected.Parts();
   while(fragment<parts.size() && parts[fragment].kind!=PIONEER_Q38_WEIGHT)fragment++;
   const auto& p=parts.at(fragment);const unsigned count=std::min(256U,p.allocated/4096-page);
   assert(r.version==1 && !r.reserved && r.bank==p.bank && r.offset==p.proposedAddress-0x04000000U+uint64_t(page)*4096 && r.bytes==count*4096);
   for(unsigned n=0;n<count;n++){
    const auto* raw=reinterpret_cast<const uint8_t*>(r.data+n*1024);const unsigned value=(fragment+page+n)%255+1;
    assert(raw[0]==value && raw[4095]==value);
   }
   uploads++;bytes+=r.bytes;page+=count;if(page==p.allocated/4096){fragment++;page=0;}
   r.result={1,TT_RUNTIME_PASSED,count*4096,count,uploads,1,1,1,0,0,0,0};
   if(uploads==1 && sweep.Clone([&]{r.result.nonce++;}))return true;
  }else if(op==TT_PCI_PROBE_Q38_SPAN_END){
   assert(size==sizeof(tt_q38_span_end) && bytes==source.bytes && source.reads==source.pages && !page && !spanEnds++ && !starts);
   *static_cast<tt_q38_span_end*>(data)={1,0,1,0};
  }else if(op==TT_PCI_PROBE_Q38_TOKEN_VALIDATE){
   namespace P=PioneerQ38TokenControl;
   assert(size==sizeof(P::Packet) && spanEnds==1);auto& p=*static_cast<P::Packet*>(data);
   assert(PioneerQ38TokenValidation::RequestShape(p));const unsigned operation=p.request.operation;
   auto& r=p.reply;r.version=1;r.operation=operation;r.started=r.ready=1;
   auto* d=r.detail;d[0]=d[1]=1;
   if(operation==P::Start){assert(!starts++ && !holds && !ends);r.stage=d[2]=8;d[3]=1;}
   else if(operation==P::Observe){
    assert(starts==1 && !holds && !ends);polls++;
    if(polls==1){r.ready=0;r.stage=d[2]=9;}
    else {r.stage=d[2]=11;d[6]=1;d[8]=3;d[9]=2;d[10]=64;d[12]=d[13]=0x60004;}
   }else if(operation==P::Hold){assert(polls==2 && !holds++ && !reads && !ends);r.stage=d[2]=4;d[6]=1;d[7]=120;}
   else if(operation==P::Read){
    assert(holds==1 && !reads++ && !ends && p.request.records==3);r.stage=d[2]=4;r.records=3;
    d[6]=1;d[8]=3;d[9]=1;d[10]=768;d[11]=1;
    for(unsigned pos=0;pos<3;pos++){
     auto* v=p.records+pos*64;v[0]=0x31435451;v[1]=1;v[2]=pos;v[3]=pos==0?248319:pos==1?17:101;
     v[4]=100+pos;v[5]=0x3f800000;v[6]=pos+1;v[7]=(pos+1)&1;v[8]=unsigned(pos==0);v[9]=unsigned(pos<2);v[10]=64;v[11]=973;
     for(unsigned i=12;i<36;i++)v[i]=i+100;
     v[44]=pos+1;v[45]=48;v[46]=119;
    }
    for(unsigned word:{0U,3U,4U,5U,8U,9U,10U,11U,12U,38U,44U,45U,46U,47U}){
     if(sweep.Clone([&]{p.records[64+word]^=word==5?0x40000000U:1U;}))return true;
    }
   }else if(operation==P::Finish){assert(reads==1 && !ends++);r.stage=d[2]=11;r.complete=1;d[3]=d[5]=1;}
   else assert(!"no implicit cancellation,old compute or Work fallback");
   for(unsigned field:{0U,1U,3U,4U,5U,7U,8U,9U})if(sweep.Clone([&]{reinterpret_cast<uint32_t*>(&r)[field]^=1;}))return true;
   if(sweep.Clone([&]{p.request.operation^=1;}))return true;
   if(sweep.Clone([&]{p.prompt[19]=1;}))return true;
   if(operation!=P::Read && sweep.Clone([&]{p.records[4095]=1;}))return true;
  }else assert(!"no legacy upload/compute/END path");
  if((op!=TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || uploads==1) && sweep.Clone([]{}))return false;
  if((op!=TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || uploads==1) && sweep.Clone([&]{source.valid=false;}))return true;
  return true;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;Sweep sweep;SyntheticSource source(fixture,sweep);SyntheticIO io(source);
 auto session=std::make_unique<PioneerQ38TokenValidationSession<SyntheticIO,SyntheticSource>>(io,source);
 std::array<uint32_t,192> out;out.fill(0xaaaaaaaa);
 assert(!session->Finish() && !session->Poll() && !session->CopyRecords(out.data(),192) && !io.opens);
 bool okay=session->Begin();
 if(okay)for(unsigned abandon=0;abandon<2;abandon++)if(sweep.Clone([]{})){
  const auto calls=io.calls;
  if(abandon)session->Abort();session.reset();
  assert(io.calls==calls && io.closes==1 && !io.polls && !io.holds && !io.reads && !io.ends);_exit(0);
 }
 if(okay){assert(!session->Begin() && !session->Finish() && !session->CopyRecords(out.data(),192));okay=session->Poll();}
 if(okay){assert(!session->TerminalReady() && !session->Finish());okay=session->Poll();}
 if(okay){assert(session->TerminalReady() && !session->CopyRecords(out.data(),192));okay=session->Finish();}
 if(sweep.child){
  assert(!okay && io.calls==sweep.fence && io.closes==1 && !session->Completed());
  auto calls=io.calls,closes=io.closes;auto reads=source.reads;
  assert(!session->Begin() && !session->Poll() && !session->Finish() && !session->CopyRecords(out.data(),192));
  session->Abort();session->Abort();session.reset();
  assert(io.calls==calls && io.closes==closes && source.reads==reads);
  for(auto word:out)assert(word==0xaaaaaaaa);_exit(0);
 }
 assert(okay && session->Completed() && io.closes==1 && io.starts==1 && io.polls==2 && io.holds==1 && io.reads==1 && io.ends==1);
 assert(session->partsUploaded==3551 && session->pagesUploaded==source.pages && source.reads==source.pages && io.bytes==source.bytes && io.progress==source.bytes);
 assert(!session->CopyRecords(out.data(),191) && session->CopyRecords(out.data(),192));
 assert(out[4]==100 && out[64+4]==101 && out[128+3]==101 && out[128+4]==102);
 assert(!session->Begin() && !session->Poll() && !session->Finish());session->Abort();session.reset();assert(io.closes==1);
 unsigned early=0;
 for(unsigned fault=0;fault<3;fault++){
  Sweep before;SyntheticSource original(fixture,before);SyntheticIO transport(original);
  original.openFail=fault==0;original.valid=fault!=1;transport.openFail=fault==2;
  auto rejected=std::make_unique<PioneerQ38TokenValidationSession<SyntheticIO,SyntheticSource>>(transport,original);
  assert(!rejected->Begin() && !transport.calls && !transport.closes && transport.opens==unsigned(fault==2));
  rejected.reset();assert(!transport.calls && !transport.closes);early++;
 }
 unsigned preflights=0;
 for(unsigned fault=0;fault<3;fault++){
  Sweep before;before.enabled=false;SyntheticSource original(fixture,before);SyntheticIO transport(original);
  auto checked=std::make_unique<PioneerQ38TokenValidationSession<SyntheticIO,SyntheticSource>>(transport,original);
  assert(checked->Preflight() && !checked->Preflight() && !transport.opens && !transport.calls);
  original.readFail=fault==2;
  const bool sampled=checked->InspectLearnedPageSamples();assert(sampled==bool(fault!=2));
  assert(original.reads==(fault==2?1U:10653U) && !transport.opens && !transport.calls && !transport.closes);
  if(fault==0){transport.openFail=true;assert(!checked->Begin() && transport.opens==1);}
  if(fault==1){original.valid=false;assert(!checked->Begin() && !transport.opens);}
  checked.reset();assert(original.opens==1 && !transport.calls && !transport.closes);preflights++;
 }
 printf("PASS actual USER full851 uploader/all3551 learned parts/%llu SYNTHETIC pages/%u batches/includes embedding+FP32/no mutable upload/SAME FD+generation/pending->terminal->HOLD->READ3->FINISH once/%u COW transport+source+reply+record+abandon+close faults/%u early source+open refusals/%u source-only sample+reuse+loss checks ZERO device calls/CLOSE_ONLY/no partial output;MOCK source+OS replies,NOT SHA authentication,native execution,model numerics,physical or TPS proof\n",(unsigned long long)source.pages,io.uploads,sweep.clones,early,preflights);
}
