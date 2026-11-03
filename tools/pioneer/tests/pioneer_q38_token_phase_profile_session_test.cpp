/* SPDX-License-Identifier: MIT */
// Actual USER session, synthetic source/OS/transport ONLY. No native owner.
#define PIONEER_Q38_DECODE_BENCHMARK_FIXTURE_MACHINE_ONLY
#define PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT 1
#define PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT 1
#define PIONEER_Q38_FRONTIER_FAILURE_REPORT_PARENT_BINDING 1
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT 1
#include "pioneer_q38_token_decode_benchmark_test.cpp"
#include "../PioneerQ38TokenPhaseProfileCli.h"
#include <functional>
namespace C=PioneerQ38TokenPhaseProfileControl;
namespace Q=PioneerQ38TokenPhaseProfileData;
static void FillMetric(C::Packet& p,bool invalid){
 uint64_t now=UINT64_C(1)<<40;Q::Initialize(p.words,true,now);
 auto mark=[&](unsigned next){now+=19;Q::Advance(p.words,true,now,next);};
 for(unsigned token=0;token<34;token++){
  for(unsigned layer=0;layer<64;layer++){mark(Q::Metadata);mark(layer%4==3?Q::Dense:Q::Recurrent);}
  for(unsigned next:{Q::HeadNormalize,Q::HeadMatrix,Q::Feedback,Q::StateCommit,Q::Publication})mark(next);
 }
 mark(Q::FinalStop);Q::Advance(p.words,true,now+19,Q::Phases,34);assert(Q::Closed(p.words));
 if(invalid){p.words[Q::State]=Q::Invalid;p.words[Q::Error]=Q::ClockUnstable;}
 auto& r=p.reply;r.version=1;r.operation=C::Read;r.started=r.ready=1;
 auto* d=r.detail;d[0]=d[1]=d[6]=1;d[2]=4;d[4]=59;d[7]=!invalid;d[8]=34;d[9]=60;
 d[10]=p.words[Q::State];d[11]=p.words[Q::Error];assert(C::ReplyShape(p));
}
struct PhaseIO:IO {
 using IO::IO;unsigned phaseReads=0,damage=96;bool invalid=false,failRead=false,failFinish=false;
 std::function<void()> duringRead;
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  if(op==TT_PCI_PROBE_Q38_TOKEN_PHASE_PROFILE_READ){
   assert(fd==76 && opens==1 && !closes && holds==1 && reads==1 && !ends && !phaseReads++ && bytes==sizeof(C::Packet));
   calls++;auto& p=*static_cast<C::Packet*>(data);assert(C::RequestShape(p));
   if(duringRead)duringRead();
   if(failRead)return false;FillMetric(p,invalid);
   if(damage<96)reinterpret_cast<uint32_t*>(&p)[damage]^=1;return true;
  }
  if(op==TT_PCI_PROBE_Q38_TOKEN_FRONTIER_BENCHMARK && static_cast<P::Packet*>(data)->request.operation==P::Finish){
   assert(phaseReads==1);if(failFinish){calls++;return false;}
  }
  return IO::Call(fd,op,data,bytes);
 }
};
int main(){
 (void)&Protocol;setvbuf(stdout,nullptr,_IOLBF,0);
 using Mode=PioneerQ38TokenPhaseProfileMode;
#ifdef PIONEER_Q38_TOKEN_MANUAL_TTSYNC_CLIENT
 const char* run[]={"manual-ttsync","--benchmark-manual-ttsync-34-positions",nullptr};
 const char* oldRun[]={"constant-q4","--benchmark-constant-q4-34-positions",nullptr};
 assert(PioneerQ38TokenPhaseProfileArguments(2,oldRun)==Mode::Invalid);
#elif defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT)
 const char* run[]={"constant-q4","--benchmark-constant-q4-34-positions",nullptr};
 const char* oldRun[]={"phase","--profile-card-phases-34-positions",nullptr};
 assert(PioneerQ38TokenPhaseProfileArguments(2,oldRun)==Mode::Invalid);
#else
 const char* run[]={"profile","--profile-card-phases-34-positions",nullptr};
#endif
 const char* sourceOnly[]={"profile","--preflight-source-only",nullptr};
 assert(PioneerQ38TokenPhaseProfileArguments(2,run)==Mode::Positions34 && PioneerQ38TokenPhaseProfileArguments(2,sourceOnly)==Mode::SourceOnly);
 unsigned cli=0,branches=0;
 for(const char* text:{"","--diagnose-frontier-34-positions","--benchmark-34-positions","--profile-frontends-34-positions",
  "--generate-34-positions","--work","--cpu","--profile-card-phases-34-positions=1","--profile-card-phases-34-positions "}){
  const char* bad[]={"profile",text,nullptr};assert(PioneerQ38TokenPhaseProfileArguments(2,bad)==Mode::Invalid);cli++;
 }
 assert(PioneerQ38TokenPhaseProfileArguments(2,nullptr)==Mode::Invalid);
 Fixture fixture;Source source(fixture);PhaseIO io(source);
 auto session=std::make_unique<PioneerQ38TokenHiddenValidationSession<PhaseIO,Source>>(io,source);
 C::Packet before;assert(!session->CopyPhaseProfile(before) && !io.calls && C::RequestShape(before));
 assert(session->Begin() && io.uploads==20367 && source.pages==4736090 && io.bytes==UINT64_C(19399024640));
 assert(!session->CopyPhaseProfile(before) && !io.phaseReads && C::RequestShape(before));
 auto child=[&](auto check){fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){check();_exit(0);}int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));branches++;};
 auto terminal=[&](){assert(session->Poll() && session->Poll() && session->TerminalReady());
  PioneerQ38TokenFrontierControl::Packet metric;assert(session->ObserveFrontier(metric));
  const unsigned calls=io.calls;assert(!session->CopyPhaseProfile(before) && io.calls==calls && C::RequestShape(before));};
 for(unsigned damage=0;damage<96;damage++)child([&]{
  terminal();io.damage=damage;assert(!session->Finish() && !session->Completed() && io.phaseReads==1 && io.holds==1 && io.reads==1 && !io.ends && io.closes==1);
  const auto calls=io.calls;assert(!session->CopyPhaseProfile(before) && !session->Finish() && io.calls==calls && C::RequestShape(before));
  session.reset();assert(io.calls==calls && io.closes==1);
 });
 for(unsigned failure=0;failure<3;failure++)child([&]{
  terminal();io.failRead=failure==0;io.failFinish=failure==1;io.failCopyClose=failure==2;
  assert(!session->Finish() && !session->Completed() && io.phaseReads==1 && io.closes==1 && io.ends==unsigned(failure==2));
  const auto calls=io.calls;assert(!session->CopyPhaseProfile(before) && C::RequestShape(before));session.reset();assert(io.calls==calls);
 });
 child([&]{
  terminal();io.invalid=true;assert(session->Finish() && session->Completed() && io.ends==1 && io.closes==1);
  C::Packet p;const auto calls=io.calls;assert(session->CopyPhaseProfile(p) && C::ReplyShape(p) && !p.reply.detail[7] && io.calls==calls);
  session.reset();assert(io.calls==calls && io.phaseReads==1);
 });
 terminal();io.duringRead=[&]{C::Packet p;const unsigned calls=io.calls;assert(!session->CopyPhaseProfile(p) && C::RequestShape(p) && io.calls==calls);};
 assert(session->Finish() && session->Completed() && io.phaseReads==1 && io.holds==1 && io.reads==1 && io.ends==1 && io.closes==1);
 C::Packet p;const auto calls=io.calls;assert(session->CopyPhaseProfile(p) && C::ReplyShape(p) && p.reply.detail[7] && io.calls==calls);
 for(unsigned repeat=0;repeat<4;repeat++){C::Packet again;assert(session->CopyPhaseProfile(again) && !memcmp(&again,&p,sizeof(p)) && io.calls==calls);}
 C::Packet dirty;dirty.words[0]=1;const auto saved=dirty;assert(!session->CopyPhaseProfile(dirty) && !memcmp(&saved,&dirty,sizeof(dirty)) && io.calls==calls);
 source.current=false;C::Packet stale;assert(!session->CopyPhaseProfile(stale) && C::RequestShape(stale) && io.calls==calls);
 session.reset();assert(io.calls==calls && io.closes==1);
 printf("PASS actual phase USER session/full851 synthetic uploader+34records/%u COW all96packet-word+transport+END+Close+invalid-clock branches/%u CLI alias refusals/ONE extra read ONLY after HOLD+drain and BEFORE END/no copied metric until wholeEND+Close/source epoch+pristine output required/invalid timing still cleans/no implicit retry or destructor IO;SYNTHETIC_ONLY_NOT_NATIVE_MODEL_PHYSICAL_TIMING_WORK_PROOF\n",branches,cli);
}
