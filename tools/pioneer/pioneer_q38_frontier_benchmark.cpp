/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <OS.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <memory>
#include "PioneerQ38FrontierFailureReportRole.h"
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT
#include "PioneerQ38FrontierFailureReport.h"
#include "PioneerQ38FrontierFailureReportCli.h"
#endif
#include "PioneerQ38TokenHiddenValidationSession.h"
#include "PioneerQ38TokenRateSamples.h"
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT
// Own explicit CLI was bound above;never expose the old benchmark spelling.
#elif defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT)
#include "PioneerQ38FrontendProfileCli.h"
#else
#include "PioneerQ38FrontierBenchmarkCli.h"
#endif
struct FrontierBenchmarkTransport {
 uint64_t reported=0;
 int Open(){puts("Q38_FRONTIER_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT");return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  errno=0;const int status=ioctl(fd,op,data,bytes),error=errno;
  if(status){
   fprintf(stderr,"Q38_FRONTIER_IOCTL_FAILED op%08x status%d errno%d;NO_RETRY\n",unsigned(op),status,error);
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT
   (void)PioneerQ38WriteFrontierFailedReply(stderr,op,data,bytes,status,error);
#endif
  }
  return status==0;
 }
 int Close(int fd){return close(fd);}
 void Progress(uint64_t done,uint64_t total){
  if(done==total || done-reported>=UINT64_C(64)*1024*1024){
   printf("Q38_FRONTIER_UPLOADED %llu/%llu\n",(unsigned long long)done,(unsigned long long)total);reported=done;
  }
 }
};
int main(int argc,char** argv){
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT
 using M=PioneerQ38FrontierFailureReportMode;const auto mode=PioneerQ38FrontierFailureReportArguments(argc,argv);
#elif defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT)
 using M=PioneerQ38FrontendProfileMode;const auto mode=PioneerQ38FrontendProfileArguments(argc,argv);
#else
 using M=PioneerQ38FrontierBenchmarkMode;const auto mode=PioneerQ38FrontierBenchmarkArguments(argc,argv);
#endif
 if(mode==M::Invalid)return 2;
 namespace V=PioneerQ38TokenDecodeBenchmark;
 setvbuf(stdout,nullptr,_IOLBF,0);
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT
 puts("Q38_FRONTIER_FAILURE_REPORT_ZERO_EXTRA_ACCELERATOR_CALLS_RAW_REPLY_NOT_ROOT_CAUSE_OR_CLEAN_END");
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 puts("Q38_FRONTEND_PROFILE_ZERO_EXTRA_ACCELERATOR_CALLS_RAW_NOT_RETIRED_PC_OR_CYCLES");
#endif
 puts("Q38_FRONTIER_FIXED_2PROMPT_34POSITION_NO_CPU_REFERENCE_NOT_ASSISTANT_WORK_NUMERICAL_VALIDATION_REQUIRED");
 PioneerQ38PublicModelSource source;FrontierBenchmarkTransport io;
 auto session=std::make_unique<PioneerQ38TokenHiddenValidationSession<FrontierBenchmarkTransport>>(io,source);
 if(mode==M::SourceOnly){
  const bool okay=session->Preflight() && session->InspectLearnedPageSamples();
  printf("%s_Q38_FRONTIER_SOURCE_ONLY reason%s;NO_ACCELERATOR_OPEN_OR_INFERENCE\n",okay?"PASS":"FAIL",session->failure?session->failure:"none");
  return okay?0:1;
 }
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 auto frontends=std::make_unique<PioneerQ38TokenFrontendSamples>();
#endif
 if(!session->Begin()){
  printf("FAIL_Q38_FRONTIER reason%s op%08x;CLOSE_ONLY_NO_RETRY_OR_IMPLICIT_END\n",session->failure?session->failure:"begin",session->operation);return 1;
 }
 puts("Q38_FRONTIER_START_ACKNOWLEDGED;NOT_COMPLETED_POSITION_OR_TOKEN");
 PioneerQ38TokenRateSamples samples(V::Prompts,V::Positions);
 const bigtime_t began=system_time();bigtime_t previous=began;
 bool okay=began>=0;const char* why=okay?nullptr:"clock";unsigned observations=0,finalCount=0;
 for(unsigned polls=0;okay;polls++){
  const bigtime_t before=system_time();
  if(before<previous || before-began>=INT64_C(1800000000) || polls>=18000){why="deadline_or_clock";okay=false;break;}
  PioneerQ38TokenFrontierControl::Packet metric;
  if(!session->ObserveFrontier(metric)){okay=false;break;}
  const bigtime_t after=system_time();
  if(!samples.Observe(metric,before,after)){why="counter_or_clock";okay=false;break;}
  observations++;finalCount=metric.reply.detail[7];previous=after;
  if(session->TerminalReady())break;
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
  const bigtime_t pollBegin=system_time();
#endif
  if(!session->Poll()){okay=false;break;}
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
  const bigtime_t pollEnd=system_time();PioneerQ38TokenFrontendSnapshot raw;
  if(pollBegin<after || !session->LastPollFrontends(raw) || !frontends->Observe(pollBegin,pollEnd,finalCount,raw)){
   why="frontend_snapshot_or_clock";okay=false;break;
  }
#endif
  if(!session->TerminalReady() && snooze(100000)!=B_OK){why="snooze";okay=false;break;}
 } // after terminal publication,one final SAME-FD metric read BEFORE HOLD
 if(okay && (!session->TerminalReady() || finalCount!=V::Positions)){why="final_count";okay=false;}
 if(okay){puts("Q38_FRONTIER_TERMINAL_READY_BEGIN_HOLD_DRAIN_AND_RESTORE;NOT_CLEAN_END_YET");okay=session->Finish();}
 auto records=std::make_unique<uint32_t[]>(V::Positions*64);
 if(okay)okay=session->CopyGenerationRecords(records.get(),V::Positions*64);
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 if(okay && !frontends->Complete()){why="frontend_terminal_missing";okay=false;}
#endif
 if(!okay){
  printf("FAIL_Q38_FRONTIER reason%s op%08x;CLOSE_ONLY_NO_RETRY_OR_IMPLICIT_END\n",why?why:session->failure?session->failure:"records",session->operation);return 1;
 }
 for(unsigned pos=0;pos<V::Positions;pos++)for(unsigned row=0;row<8;row++){
  printf("Q38_FRONTIER_RECORD position%u word%02u",pos,row*8);
  for(unsigned col=0;col<8;col++)printf(" %08x",records[pos*64+row*8+col]);
  putchar('\n');
 }
 printf("PASS_Q38_FRONTIER uploaded_parts%u pages%llu spans%u records34 final_count%u clean_end1;EXTERNAL_NUMERICAL_COMPARISON_REQUIRED\n",
  session->partsUploaded,(unsigned long long)session->pagesUploaded,session->uploadSpans,finalCount);
 PioneerQ38TokenRateSamples::Bounds bounds;
 if(samples.ObservedBounds(bounds) && bounds.sampledTokens>=16){
  printf("Q38_FRONTIER_OBSERVED_COUNT_RATE_BOUNDS sampled_tokens%u first_committed%u last_committed%u elapsed_min_us%lld elapsed_max_us%lld lower_tokens_per_sec%.9f upper_tokens_per_sec%.9f observations%u;POST_PROMPT_COUNT_WINDOW_NOT_ISOLATED_TOKEN_LATENCY_OR_QUALITY_PROOF\n",
   bounds.sampledTokens,bounds.firstCommitted,bounds.lastCommitted,(long long)bounds.minElapsedUs,(long long)bounds.maxElapsedUs,
   bounds.lowerTokensPerSecond,bounds.upperTokensPerSecond,observations);
 }else puts("Q38_FRONTIER_RATE_UNAVAILABLE_INSUFFICIENT_POST_PROMPT_WINDOW;CLEAN_END_DOES_NOT_PROVE_RATE");
#ifdef PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT
 // Print raw observations ONLY after genuine whole Finish+Close+records.
 // The preceding count and Observe are separate calls,not an atomic snapshot.
 printf("Q38_FRONTEND_ROWS_BEGIN generation%llu rows%u;AFTER_CLEAN_END_NOT_ATOMIC_WITH_PRECEDING_COUNT\n",
  (unsigned long long)frontends->Generation(),frontends->Size());
 for(unsigned i=0;i<frontends->Size();i++){
  const auto& row=frontends->Data()[i];const auto& raw=row.observed;
  printf("Q38_FRONTEND_ROW index%u begin_us%lld end_us%lld preceding_count%u ready%u stage%u checked_word%u first_raw%08x second_raw%08x\n",
   i,(long long)row.begin,(long long)row.end,row.precedingCount,raw.ready,raw.stage,raw.checkedWord,raw.firstPC,raw.secondPC);
 }
 printf("Q38_FRONTEND_ROWS_END rows%u;FRONTEND_SAMPLE_COUNTS_NOT_RETIRED_INSTRUCTION_CYCLES_OR_FUNCTION_TIME\n",frontends->Size());
#endif
 return 0;
}
