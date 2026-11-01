/* SPDX-License-Identifier: MIT */
// SAME real USER session template,synthetic source/OS/transport ONLY.
// Failure replies below are synthetic inputs,NOT GPU products or causes.
#define PIONEER_Q38_DECODE_BENCHMARK_FIXTURE_MACHINE_ONLY
#define PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT 1
#define PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT 1
#define PIONEER_Q38_FRONTIER_FAILURE_REPORT_PARENT_BINDING 1
#include "pioneer_q38_token_decode_benchmark_test.cpp"
#include "../PioneerQ38FrontierFailureReport.h"
struct FailureReportIO:IO {
 using IO::IO;
 FILE* sink=nullptr;
 bool report=false;
 unsigned failKind=0,cursor=0,reports=0;
 bool Call(int fd,unsigned op,void* data,size_t bytes){
  if(op!=TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE || !failKind)return IO::Call(fd,op,data,bytes);
  namespace F=PioneerQ38TokenFrontierControl;
  assert(fd==76 && opens==1 && !closes && source.Ready() && starts==1 && !holds && !ends
   && bytes==sizeof(F::Packet));calls++;metrics++;
  auto& p=*static_cast<F::Packet*>(data);assert(F::RequestShape(p));
  if(failKind!=2){
   auto& r=p.reply;r.version=1;r.operation=F::Observe;r.status=int32_t(0x8000a00cU);r.started=r.unsafe=1;
   r.detail[0]=r.detail[1]=r.detail[5]=1;r.detail[2]=12;r.detail[4]=cursor;
   if(failKind==3)r.reserved[1]=1; // copied malformed packet,still RAW only
  } // failKind2 models ioctl rejection BEFORE any reply is copied.
  const auto before=p;
  if(report){assert(PioneerQ38WriteFrontierFailedReply(sink,op,data,bytes,-1,int32_t(0x8000a00cU)));reports++;}
  assert(!std::memcmp(&before,&p,sizeof(p)));return false;
 }
};
int main(){
 (void)&Protocol;setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;Source source(fixture);FailureReportIO io(source);
 auto session=std::make_unique<PioneerQ38TokenHiddenValidationSession<FailureReportIO,Source>>(io,source);
 assert(session->Begin() && io.uploads==20367 && source.pages==4736090 && io.bytes==UINT64_C(19399024640));
 unsigned branches=0;
 for(unsigned phase=0;phase<3;phase++)for(unsigned kind=1;kind<=3;kind++)for(unsigned cursor:{0U,3U,63U})for(bool report:{false,true}){
  fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   for(unsigned i=0;i<phase;i++)assert(session->Poll());
   assert(session->TerminalReady()==(phase==2));
   const auto calls=io.calls,metrics=io.metrics,polls=io.polls,starts=io.starts,uploads=io.uploads;
   io.sink=std::tmpfile();assert(io.sink);io.report=report;io.failKind=kind;io.cursor=cursor;
   PioneerQ38TokenFrontierControl::Packet output;const auto before=output;
   assert(!session->ObserveFrontier(output) && !std::memcmp(&before,&output,sizeof(output))
    && io.calls==calls+1 && io.metrics==metrics+1 && io.closes==1 && !io.holds && !io.ends
    && io.polls==polls && io.starts==starts && io.uploads==uploads && io.reports==unsigned(report));
   assert(!std::fflush(io.sink) && std::ftell(io.sink)>=0);
   assert((std::ftell(io.sink)>0)==report);std::fclose(io.sink);io.sink=nullptr;
   const auto stopped=io.calls;
   std::array<uint32_t,V::Positions*64> records;records.fill(0xa5a5a5a5);
   assert(!session->ObserveFrontier(output) && !session->Poll() && !session->Finish()
    && !session->RequestCancellation() && !session->Completed()
    && !session->CopyGenerationRecords(records.data(),records.size()) && io.calls==stopped);
   for(auto word:records)assert(word==0xa5a5a5a5);
   session.reset();assert(io.calls==stopped && io.closes==1 && !io.holds && !io.ends);_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));branches++;
 }
 // Original parent completes normally,logging enabled but NEVER invoked.
 io.sink=std::tmpfile();assert(io.sink);io.report=true;
 PioneerQ38TokenFrontierControl::Packet metric;
 assert(session->ObserveFrontier(metric) && session->Poll() && session->Poll()
  && session->TerminalReady() && session->ObserveFrontier(metric) && session->Finish() && session->Completed());
 std::array<uint32_t,V::Positions*64> records;assert(session->CopyGenerationRecords(records.data(),records.size()));
 assert(io.metrics==2 && io.polls==2 && io.holds==1 && io.ends==1 && io.closes==1 && !io.reports
  && !std::fflush(io.sink) && std::ftell(io.sink)==0);std::fclose(io.sink);
 const auto calls=io.calls;session.reset();assert(io.calls==calls && io.closes==1);
 printf("PASS failed-reply SAME-session/%u COW report-on-vs-off failures/early-no-copy+unsafe+malformed/first+running+terminal observation/one failed call+one Close/no retry,Cancel,HOLD,END or destructor IO/parent ALL34 records+HOLD+END+Close with ZERO reports;SYNTHETIC_ONLY_NOT_NATIVE_HARDWARE_CAUSE_WORK_OR_TPS\n",branches);
}
