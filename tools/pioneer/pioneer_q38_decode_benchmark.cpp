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
#include "PioneerQ38DecodeBenchmarkCli.h"
struct DecodeBenchmarkTransport {
 uint64_t reported=0;bigtime_t opened=-1,startBegin=-1,startReturned=-1;
 uint64_t uploadCalls=0,uploadUs=0;bool timing=true;
 int Open(){
  opened=system_time();
  puts("Q38_DECODE_BENCHMARK_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT");
  return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);
 }
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  const auto began=system_time();bool starting=false;
  if(op==TT_PCI_PROBE_Q38_TOKEN_DECODE_BENCHMARK){
   const auto action=static_cast<PioneerQ38TokenControl::Packet*>(data)->request.operation;
   starting=action==PioneerQ38TokenControl::Start;
   if(starting)startBegin=began;
  }
  errno=0;const int status=ioctl(fd,op,data,bytes),error=errno;
  const auto ended=system_time();
  if(began<0 || ended<began)timing=false;
  else if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD){uploadCalls++;uploadUs+=uint64_t(ended-began);}
  if(starting)startReturned=ended;
  if(status)printf("Q38_DECODE_BENCHMARK_IOCTL_FAILED op%08x status%d errno%d;NO_RETRY\n",unsigned(op),status,error);
  return status==0;
 }
 int Close(int fd){return close(fd);}
 void Progress(uint64_t done,uint64_t total){
  if(done==total || done-reported>=UINT64_C(64)*1024*1024){
   printf("Q38_DECODE_BENCHMARK_UPLOADED %llu/%llu\n",(unsigned long long)done,(unsigned long long)total);reported=done;
  }
 }
};
int main(int argc,char** argv){
 using M=PioneerQ38DecodeBenchmarkMode;
 const auto mode=PioneerQ38DecodeBenchmarkArguments(argc,argv);
 if(mode==M::Invalid)return 2; // no source authentication or device Open
 namespace V=PioneerQ38TokenDecodeBenchmark;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("Q38_DECODE_BENCHMARK_FIXED_2PROMPT_34POSITION_NO_CPU_MODEL_REFERENCE_NOT_ASSISTANT_WORK_OR_STEADY_TOKEN_RATE");
 const bigtime_t entry=system_time();
 PioneerQ38PublicModelSource source;DecodeBenchmarkTransport io;
 auto session=std::make_unique<PioneerQ38TokenHiddenValidationSession<DecodeBenchmarkTransport>>(io,source);
 if(mode==M::SourceOnly){
  const bool okay=session->Preflight() && session->InspectLearnedPageSamples();
  printf("%s_Q38_DECODE_BENCHMARK_SOURCE_ONLY reason%s;NO_ACCELERATOR_OPEN_OR_INFERENCE\n",
   okay?"PASS":"FAIL",session->failure?session->failure:"none");return okay?0:1;
 }
 if(!session->Begin()){
  printf("FAIL_Q38_DECODE_BENCHMARK reason%s op%08x;CLOSE_ONLY_NO_RETRY_OR_IMPLICIT_END\n",
   session->failure?session->failure:"begin",session->operation);return 1;
 }
 puts("Q38_DECODE_BENCHMARK_START_ACKNOWLEDGED;NOT_COMPLETED_POSITION_OR_TOKEN");
 const bigtime_t began=system_time();bigtime_t previous=began;
 bool okay=began>=0;const char* why=okay?nullptr:"clock";
 for(unsigned polls=0;okay && !session->TerminalReady();polls++){
  const bigtime_t now=system_time();
  if(now<previous || now-began>=INT64_C(1800000000) || polls>=18000){why="deadline_or_clock";okay=false;break;}
  previous=now;
  if(!session->Poll()){okay=false;break;}
  if(!session->TerminalReady() && snooze(100000)!=B_OK){why="snooze";okay=false;break;}
 }
 const bigtime_t terminal=okay?system_time():-1;
 if(okay){puts("Q38_DECODE_BENCHMARK_TERMINAL_READY_BEGIN_HOLD_DRAIN_AND_RESTORE;NOT_CLEAN_END_YET");okay=session->Finish();}
 const bigtime_t closed=okay?system_time():-1;
 auto records=std::make_unique<uint32_t[]>(V::Positions*64);
 if(okay)okay=session->CopyGenerationRecords(records.get(),V::Positions*64);
 if(!okay){
  printf("FAIL_Q38_DECODE_BENCHMARK reason%s op%08x;CLOSE_ONLY_NO_RETRY_OR_IMPLICIT_END\n",
   why?why:session->failure?session->failure:"record_publication",session->operation);return 1;
 }
 for(unsigned pos=0;pos<V::Positions;pos++)for(unsigned row=0;row<8;row++){
  printf("Q38_DECODE_BENCHMARK_RECORD position%u word%02u",pos,row*8);
  for(unsigned col=0;col<8;col++)printf(" %08x",records[pos*64+row*8+col]);
  putchar('\n');
 }
 printf("PASS_Q38_DECODE_BENCHMARK uploaded_parts%u pages%llu spans%u records34 clean_end1;EXTERNAL_NUMERICAL_COMPARISON_REQUIRED_NOT_STEADY_TOKEN_RATE\n",
  session->partsUploaded,(unsigned long long)session->pagesUploaded,session->uploadSpans);
 if(io.timing && entry>=0 && io.opened>=entry && io.startBegin>=io.opened
  && io.startReturned>=io.startBegin && terminal>=io.startReturned && closed>=terminal){
  fprintf(stderr,"Q38_DECODE_BENCHMARK_HOST_WALL_US source_to_open%lld open_to_start_begin%lld startup%lld start_return_to_terminal%lld terminal_to_close%lld total%lld upload_calls%llu upload_us%llu\n",
   (long long)(io.opened-entry),(long long)(io.startBegin-io.opened),(long long)(io.startReturned-io.startBegin),
   (long long)(terminal-io.startReturned),(long long)(closed-terminal),(long long)(closed-entry),
   (unsigned long long)io.uploadCalls,(unsigned long long)io.uploadUs);
  fputs("HOST WALL ONLY;startup includes initialization and may overlap inference;two prompt positions included;NO isolated-device-cycles or steady-decode TPS claim\n",stderr);
 }else fputs("Q38_DECODE_BENCHMARK_HOST_WALL_UNAVAILABLE;NO_RATE\n",stderr);
 return 0;
}
