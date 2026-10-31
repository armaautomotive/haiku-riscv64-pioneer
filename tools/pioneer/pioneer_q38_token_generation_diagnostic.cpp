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
#include "PioneerQ38TokenGenerationDiagnosticCli.h"
#ifdef PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING
#include "PioneerQ38GenerationHostTiming.h"
#endif

struct GenerationDiagnosticTransport {
 uint64_t reported=0;
#ifdef PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING
 PioneerQ38GenerationHostTiming timing{true};
#endif
 int Open(){
#ifdef PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING
  timing.At(PioneerQ38GenerationHostTiming::DeviceOpen,system_time());
#endif
  puts("DEBUG75_GENERATION_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT");
  return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);
 }
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
#ifdef PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING
  using T=PioneerQ38GenerationHostTiming;auto bucket=T::Other;
  if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD)bucket=T::Upload;
  else if(op==TT_PCI_PROBE_Q38_TOKEN_HIDDEN_CONTROL){
   const auto action=static_cast<PioneerQ38TokenControl::Packet*>(data)->request.operation;
   namespace P=PioneerQ38TokenControl;
   bucket=action==P::Start?T::Startup:action==P::Observe?T::Observe
    :action==P::Hold || action==P::Read || action==P::Finish?T::Cleanup:T::Other;
  }
  const bigtime_t began=system_time();
#endif
  errno=0;const int status=ioctl(fd,op,data,bytes);const int error=errno;
#ifdef PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING
  timing.Call(bucket,began,system_time());
#endif
  if(status)printf("DEBUG75_GENERATION_IOCTL_FAILED op%08x status%d errno%d;NO_RETRY\n",unsigned(op),status,error);
  return status==0;
 }
 int Close(int fd){return close(fd);}
 void Progress(uint64_t bytes,uint64_t total){
  if(bytes==total || bytes-reported>=UINT64_C(64)*1024*1024){
   printf("DEBUG75_GENERATION_UPLOADED %llu/%llu\n",(unsigned long long)bytes,(unsigned long long)total);reported=bytes;
  }
 }
};
int main(int argc,char** argv){
 const auto mode=PioneerQ38TokenGenerationDiagnosticArguments(argc,argv);
 using M=PioneerQ38TokenGenerationDiagnosticMode;
 if(mode==M::Invalid)return 2; // NO source authentication or device Open on invalid arguments
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("DEBUG75_GENERATION_FIXED_2PROMPT_3POSITION_DIAGNOSTIC_NO_CPU_MODEL_REFERENCE_NOT_ASSISTANT_WORK_OR_TOKEN_RATE");
 PioneerQ38PublicModelSource source;GenerationDiagnosticTransport io;
#ifdef PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING
 io.timing.At(PioneerQ38GenerationHostTiming::Entry,system_time());
 struct Report {
  GenerationDiagnosticTransport& io;
  ~Report(){io.timing.At(PioneerQ38GenerationHostTiming::Returned,system_time());io.timing.Print(stderr);}
 } report{io}; // stderr ONLY;does not change the fixed stdout DATA family or result
#endif
 auto session=std::make_unique<PioneerQ38TokenHiddenValidationSession<GenerationDiagnosticTransport>>(io,source);
 if(mode==M::SourceOnly){
  const bool okay=session->Preflight() && session->InspectLearnedPageSamples();
  printf("%s_DEBUG75_GENERATION_SOURCE_ONLY reason%s sampled_pages10653 planned_bytes%llu;NO_ACCELERATOR_OPEN_OR_INFERENCE\n",
   okay?"PASS":"FAIL",session->failure?session->failure:"none",(unsigned long long)session->bytesPlanned);
  return okay?0:1;
 }
 if(!session->Begin()){
  printf("FAIL_DEBUG75_GENERATION reason%s op%08x;CLOSE_ONLY_RESOURCES_MAY_BE_RETAINED\n",session->failure?session->failure:"begin",session->operation);return 1;
 }
 puts("DEBUG75_GENERATION_START_ACKNOWLEDGED;NOT_COMPLETED_POSITION_OR_TOKEN");
#ifdef PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING
 io.timing.At(PioneerQ38GenerationHostTiming::StartReturned,system_time());
#endif
 const bigtime_t began=system_time();bigtime_t previous=began;
 bool okay=began>=0;const char* why=okay?nullptr:"clock";
 for(unsigned polls=0;okay && !session->TerminalReady();polls++){
  const bigtime_t now=system_time();
  if(now<previous || now-began>=INT64_C(1800000000) || polls>=18000){why="deadline_or_clock";okay=false;break;}
  previous=now;
  if(!session->Poll()){okay=false;break;}
  if(!session->TerminalReady() && snooze(100000)!=B_OK){why="snooze";okay=false;break;}
 } // SAME healthy lifetime;NO automatic cancel,retry,reset,CPU fallback or following END on failure
 if(okay){
#ifdef PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING
  io.timing.At(PioneerQ38GenerationHostTiming::TerminalObserved,system_time());
#endif
  puts("DEBUG75_GENERATION_TERMINAL_READY_BEGIN_HOLD_DRAIN_AND_RESTORE;NOT_CLEAN_END_YET");
  okay=session->Finish();
#ifdef PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING
  if(okay)io.timing.At(PioneerQ38GenerationHostTiming::CleanClosed,system_time());
#endif
 }
 uint32_t records[192]={};
 if(okay)okay=session->CopyGenerationRecords(records,192);
 if(!okay){
  printf("FAIL_DEBUG75_GENERATION reason%s op%08x;CLOSE_ONLY_NO_RETRY_OR_IMPLICIT_END\n",
   why?why:session->failure?session->failure:"record_publication",session->operation);return 1;
 }
 for(unsigned pos=0;pos<3;pos++)for(unsigned row=0;row<8;row++){
  printf("DEBUG75_GENERATION_RECORD position%u word%02u",pos,row*8);
  for(unsigned col=0;col<8;col++)printf(" %08x",records[pos*64+row*8+col]);
  putchar('\n');
 }
 printf("PASS_DEBUG75_GENERATION uploaded_parts%u pages%llu spans%u records3 clean_end1;DATA_CONSISTENCY_ONLY_EXTERNAL_NUMERICAL_COMPARISON_REQUIRED_NOT_STEADY_TOKEN_RATE\n",
  session->partsUploaded,(unsigned long long)session->pagesUploaded,session->uploadSpans);
 return 0;
}
