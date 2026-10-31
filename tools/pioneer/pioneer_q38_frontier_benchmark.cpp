/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <OS.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <memory>
#include "PioneerQ38TokenHiddenValidationSession.h"
#include "PioneerQ38TokenRateSamples.h"
#include "PioneerQ38FrontierBenchmarkCli.h"
struct FrontierBenchmarkTransport {
 uint64_t reported=0;
 int Open(){puts("Q38_FRONTIER_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT");return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  errno=0;const int status=ioctl(fd,op,data,bytes),error=errno;
  if(status)fprintf(stderr,"Q38_FRONTIER_IOCTL_FAILED op%08x status%d errno%d;NO_RETRY\n",unsigned(op),status,error);
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
 using M=PioneerQ38FrontierBenchmarkMode;const auto mode=PioneerQ38FrontierBenchmarkArguments(argc,argv);
 if(mode==M::Invalid)return 2;
 namespace V=PioneerQ38TokenDecodeBenchmark;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("Q38_FRONTIER_FIXED_2PROMPT_34POSITION_NO_CPU_REFERENCE_NOT_ASSISTANT_WORK_NUMERICAL_VALIDATION_REQUIRED");
 PioneerQ38PublicModelSource source;FrontierBenchmarkTransport io;
 auto session=std::make_unique<PioneerQ38TokenHiddenValidationSession<FrontierBenchmarkTransport>>(io,source);
 if(mode==M::SourceOnly){
  const bool okay=session->Preflight() && session->InspectLearnedPageSamples();
  printf("%s_Q38_FRONTIER_SOURCE_ONLY reason%s;NO_ACCELERATOR_OPEN_OR_INFERENCE\n",okay?"PASS":"FAIL",session->failure?session->failure:"none");
  return okay?0:1;
 }
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
  if(!session->Poll()){okay=false;break;}
  if(!session->TerminalReady() && snooze(100000)!=B_OK){why="snooze";okay=false;break;}
 } // after terminal publication,one final SAME-FD metric read BEFORE HOLD
 if(okay && (!session->TerminalReady() || finalCount!=V::Positions)){why="final_count";okay=false;}
 if(okay){puts("Q38_FRONTIER_TERMINAL_READY_BEGIN_HOLD_DRAIN_AND_RESTORE;NOT_CLEAN_END_YET");okay=session->Finish();}
 auto records=std::make_unique<uint32_t[]>(V::Positions*64);
 if(okay)okay=session->CopyGenerationRecords(records.get(),V::Positions*64);
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
 return 0;
}
