/* SPDX-License-Identifier: MIT */
// Actual USER lifecycle;all learned source,metrics,STOP and IO are SYNTHETIC.
// Negative COW snapshots here never claim native/model/physical execution.
#define PIONEER_Q38_DECODE_BENCHMARK_FIXTURE_MACHINE_ONLY
#define PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT 1
#define PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT 1
#define PIONEER_Q38_FRONTIER_FAILURE_REPORT_PARENT_BINDING 1
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT 1
#define PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT 1
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT 1
#include "pioneer_q38_token_decode_benchmark_test.cpp"
#include "../PioneerQ38TokenMatrixPhaseCli.h"
#include <functional>
namespace M=PioneerQ38TokenMatrixPhaseControl;
namespace Q=PioneerQ38TokenMatrixPhaseData;
static M::Packet Matrix(unsigned worker,unsigned state=Q::Intervals){
 M::Packet p;p.request.worker=worker;auto& r=p.reply;
 r.version=1;r.operation=M::Read;r.started=r.ready=1;r.worker=worker;r.stopNonce=19;
 auto* d=r.detail;d[0]=d[1]=d[6]=1;d[2]=4;d[3]=worker;d[4]=63;
 if(state!=Q::Missing){
  auto* w=p.words;w[0]=Q::Magic;w[1]=Q::Version;w[2]=17;w[3]=worker;w[4]=12;w[5]=17408;w[6]=3;
  if(state==Q::Intervals){w[7]=1;w[8]=7;w[9]=3;w[10]=11;w[11]=5;w[12]=13;w[13]=7;}
  if(state==Q::Malformed)w[0]^=1;
 }
 d[7]=state==Q::Intervals;d[8]=state;d[9]=p.words[2];d[10]=p.words[5];d[11]=p.words[6];
 assert(M::ReplyShape(p));return p;
}
static void Phase(PioneerQ38TokenPhaseProfileControl::Packet& p){
 namespace H=PioneerQ38TokenPhaseProfileData;
 uint64_t now=UINT64_C(1)<<40;H::Initialize(p.words,true,now);
 auto mark=[&](unsigned next){now+=19;H::Advance(p.words,true,now,next);};
 for(unsigned token=0;token<34;token++){
  for(unsigned layer=0;layer<64;layer++){mark(H::Metadata);mark(layer%4==3?H::Dense:H::Recurrent);}
  for(unsigned next:{H::HeadNormalize,H::HeadMatrix,H::Feedback,H::StateCommit,H::Publication})mark(next);
 }
 mark(H::FinalStop);H::Advance(p.words,true,now+19,H::Phases,34);assert(H::Closed(p.words));
 auto& r=p.reply;r.version=1;r.operation=1;r.started=r.ready=1;
 auto* d=r.detail;d[0]=d[1]=d[6]=d[7]=1;d[2]=4;d[4]=59;d[8]=34;d[9]=60;d[10]=p.words[H::State];
 assert(PioneerQ38TokenPhaseProfileControl::ReplyShape(p));
}
struct MatrixIO:IO {
 using IO::IO;unsigned phaseReads=0,matrixReads=0,damage=48,target=60,failWorker=0;
 unsigned invalidState=Q::Intervals;bool failFinish=false;std::function<void(unsigned)> duringRead;
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  if(op==TT_PCI_PROBE_Q38_TOKEN_PHASE_PROFILE_READ){
   assert(fd==76 && opens==1 && !closes && holds==1 && reads==1 && !ends && !phaseReads++ && !matrixReads);
   assert(bytes==sizeof(PioneerQ38TokenPhaseProfileControl::Packet));calls++;
   auto& p=*static_cast<PioneerQ38TokenPhaseProfileControl::Packet*>(data);
   assert(PioneerQ38TokenPhaseProfileControl::RequestShape(p));Phase(p);return true;
  }
  if(op==TT_PCI_PROBE_Q38_TOKEN_MATRIX_PHASE_READ){
   assert(fd==76 && opens==1 && !closes && holds==1 && reads==1 && !ends && phaseReads==1 && matrixReads<119);
   assert(bytes==sizeof(M::Packet));auto& p=*static_cast<M::Packet*>(data);
   assert(M::RequestShape(p) && p.request.worker==matrixReads+1);const auto worker=++matrixReads;calls++;
   if(duringRead)duringRead(worker);
   if(worker==failWorker)return false;
   p=Matrix(worker,worker==target?invalidState:unsigned(Q::Intervals));
   if(worker==target && damage<48){uint32_t raw[48];memcpy(raw,&p,sizeof(raw));raw[damage]^=1;memcpy(&p,raw,sizeof(raw));}
   return true;
  }
  if(op==TT_PCI_PROBE_Q38_TOKEN_FRONTIER_BENCHMARK && static_cast<P::Packet*>(data)->request.operation==P::Finish){
   assert(phaseReads==1 && matrixReads==119);if(failFinish){calls++;return false;}
  }
  return IO::Call(fd,op,data,bytes);
 }
};
int main(){
 (void)&Protocol;setvbuf(stdout,nullptr,_IOLBF,0);
 using Mode=PioneerQ38TokenPhaseProfileMode;unsigned cli=0,branches=0;
 const char* run[]={"matrix","--profile-q4-jobs-34-positions",nullptr};
 const char* sourceOnly[]={"matrix","--preflight-source-only",nullptr};
 assert(PioneerQ38TokenPhaseProfileArguments(2,run)==Mode::Positions34
  && PioneerQ38TokenPhaseProfileArguments(2,sourceOnly)==Mode::SourceOnly);
 for(const char* flag:{"","--benchmark-constant-q4-34-positions","--profile-card-phases-34-positions",
  "--diagnose-frontier-34-positions","--benchmark-34-positions","--profile-frontends-34-positions",
  "--generate-34-positions","--work","--cpu","--profile-q4-jobs-34-positions=1","--profile-q4-jobs-34-positions "}){
  const char* bad[]={"matrix",flag,nullptr};assert(PioneerQ38TokenPhaseProfileArguments(2,bad)==Mode::Invalid);cli++;
 }
 Fixture fixture;Source source(fixture);MatrixIO io(source);
 auto session=std::make_unique<PioneerQ38TokenHiddenValidationSession<MatrixIO,Source>>(io,source);
 auto empty=[](unsigned worker){M::Packet p;p.request.worker=worker;return p;};
 M::Packet before=empty(1);assert(!session->CopyMatrixPhase(1,before) && !io.calls && M::RequestShape(before));
 assert(session->Begin() && io.uploads==20367 && source.pages==4736090 && io.bytes==UINT64_C(19399024640));
 assert(!session->CopyMatrixPhase(1,before) && !io.matrixReads && M::RequestShape(before));
 auto child=[&](auto check){fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
  if(!pid){check();_exit(0);}int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));branches++;};
 auto terminal=[&](){assert(session->Poll() && session->Poll() && session->TerminalReady());
  PioneerQ38TokenFrontierControl::Packet metric;assert(session->ObserveFrontier(metric));
  const auto calls=io.calls;assert(!session->CopyMatrixPhase(1,before) && io.calls==calls && M::RequestShape(before));};
 for(unsigned target:{1U,60U,119U})for(unsigned damage=0;damage<48;damage++){
  if(damage==15 || (damage>=40 && damage<=45))continue; // plausible changed bound/cycles are DATA,not provenance
  child([&]{terminal();io.target=target;io.damage=damage;
   assert(!session->Finish() && !session->Completed() && io.matrixReads==target && !io.ends && io.closes==1);
   const auto calls=io.calls;assert(!session->CopyMatrixPhase(1,before) && M::RequestShape(before)
    && !session->Finish() && io.calls==calls);session.reset();assert(io.calls==calls && io.closes==1);
  });
 }
 for(unsigned worker:{1U,60U,119U})child([&]{terminal();io.failWorker=worker;
  assert(!session->Finish() && !session->Completed() && io.matrixReads==worker && !io.ends && io.closes==1);
  const auto calls=io.calls;assert(!session->CopyMatrixPhase(1,before) && io.calls==calls);session.reset();assert(io.calls==calls);
 });
 for(unsigned failure=0;failure<3;failure++)child([&]{terminal();
  io.failFinish=failure==0;io.failCopyClose=failure==1;
  if(failure==2)io.duringRead=[&](unsigned worker){if(worker==60)source.current=false;};
  assert(!session->Finish() && !session->Completed() && io.closes==1
   && io.ends==unsigned(failure==1) && io.matrixReads==(failure==2?60U:119U));
  const auto calls=io.calls;assert(!session->CopyMatrixPhase(1,before) && M::RequestShape(before));session.reset();assert(io.calls==calls);
 });
 for(unsigned invalid:{unsigned(Q::Missing),unsigned(Q::InvalidClock),unsigned(Q::Malformed)})child([&]{
  terminal();io.invalidState=invalid;
  assert(session->Finish() && session->Completed() && io.matrixReads==119 && io.ends==1 && io.closes==1);
  auto p=empty(60);const auto calls=io.calls;assert(session->CopyMatrixPhase(60,p) && M::ReplyShape(p)
   && p.reply.detail[8]==invalid && !p.reply.detail[7] && io.calls==calls);session.reset();assert(io.calls==calls);
 });
 terminal();io.duringRead=[&](unsigned){auto p=empty(1);const auto calls=io.calls;
  assert(!session->CopyMatrixPhase(1,p) && M::RequestShape(p) && io.calls==calls);};
 assert(session->Finish() && session->Completed() && io.matrixReads==119 && io.ends==1 && io.closes==1);
 const auto calls=io.calls;
 for(unsigned worker=1;worker<120;worker++){
  auto p=empty(worker);assert(session->CopyMatrixPhase(worker,p) && M::ReplyShape(p) && p.reply.worker==worker && io.calls==calls);
  for(unsigned repeat=0;repeat<2;repeat++){auto again=empty(worker);
   assert(session->CopyMatrixPhase(worker,again) && !memcmp(&p,&again,sizeof(p)) && io.calls==calls);}
 }
 for(unsigned worker:{0U,120U,UINT32_MAX}){auto p=empty(1);assert(!session->CopyMatrixPhase(worker,p) && io.calls==calls);}
 auto dirty=empty(1);dirty.words[0]=1;const auto saved=dirty;
 assert(!session->CopyMatrixPhase(1,dirty) && !memcmp(&saved,&dirty,sizeof(dirty)) && io.calls==calls);
 source.current=false;auto stale=empty(1);assert(!session->CopyMatrixPhase(1,stale) && M::RequestShape(stale) && io.calls==calls);
 session.reset();assert(io.calls==calls && io.closes==1);
 printf("PASS actual sampler USER lifecycle/full851 SYNTHETIC uploader+34 records/%u negative COW protected-word,transport,END,Close,source-epoch,invalid-DATA branches/%u CLI refusals/119 fixed worker reads AFTER HOLD+drain BEFORE normal END/invalid metric still cleans/NO output before whole END+Close/no destructor retry or IO;SYNTHETIC source,metrics,STOP and transport,NOT native owner/model/physical timing/Work proof\n",branches,cli);
}
