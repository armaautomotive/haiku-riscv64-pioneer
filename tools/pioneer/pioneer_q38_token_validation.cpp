/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <OS.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include "PioneerQ38TokenValidationSession.h"

struct TokenValidationTransport {
 uint64_t reported=0;
 int Open(){
  puts("DEBUG74_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT");
  return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);
 }
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  errno=0;const int status=ioctl(fd,op,data,bytes);
  if(status)printf("DEBUG74_IOCTL_FAILED op%08x status%d errno%d;NO_RETRY\n",unsigned(op),status,errno);
  return status==0;
 }
 int Close(int fd){return close(fd);}
 void Progress(uint64_t bytes,uint64_t total){
  if(bytes==total || bytes-reported>=UINT64_C(64)*1024*1024){
   printf("DEBUG74_UPLOADED %llu/%llu\n",(unsigned long long)bytes,(unsigned long long)total);reported=bytes;
  }
 }
};
int main(int argc,char** argv){
 if(argc!=2)return 2;
 const bool preflight=!std::strcmp(argv[1],"--preflight-source-only");
 if(!preflight && std::strcmp(argv[1],"--validate-three-positions"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("DEBUG74_FIXED_2PROMPT_3POSITION_DIAGNOSTIC_NOT_ASSISTANT_WORK_OR_TOKEN_RATE");
 PioneerQ38PublicModelSource source;TokenValidationTransport io;
 auto session=std::make_unique<PioneerQ38TokenValidationSession<TokenValidationTransport>>(io,source);
 if(preflight){
  const bool okay=session->Preflight() && session->InspectLearnedPageSamples();
  printf("%s_DEBUG74_SOURCE_ONLY reason%s learned_parts3551 sampled_pages10653 planned_bytes%llu;FULL_SHA_PLUS_RAW_SAMPLES_NO_ACCELERATOR_OPEN_OR_INFERENCE\n",
   okay?"PASS":"FAIL",session->failure?session->failure:"none",(unsigned long long)session->bytesPlanned);
  return okay?0:1;
 }
 const bigtime_t before=system_time();
 if(before<0)return 1;
 if(!session->Begin()){
  printf("FAIL_DEBUG74 reason%s op%08x;CLOSE_ONLY_RESOURCES_MAY_BE_RETAINED\n",session->failure?session->failure:"begin",session->operation);return 1;
 }
 const bigtime_t began=system_time();bigtime_t previous=began;
 bool okay=began>=before;const char* observationFailure=okay?nullptr:"clock";
 // A known healthy but slow diagnostic is observed,not restarted. A lost
 // channel/clock/deadline closes ONLY;no automatic cancel/END or GPU retry.
 for(unsigned polls=0;okay && !session->TerminalReady();polls++){
  const bigtime_t now=system_time();
  if(now<previous || now-began>=INT64_C(1800000000) || polls>=18000){observationFailure="deadline_or_clock";okay=false;break;}
  previous=now;
  if(!session->Poll()){okay=false;break;}
  if(!session->TerminalReady() && snooze(100000)!=B_OK){observationFailure="snooze";okay=false;break;}
 }
 if(okay)okay=session->Finish();
 uint32_t records[3*64]={};if(okay)okay=session->CopyRecords(records,3*64);
 if(!okay){
  session->Abort();printf("FAIL_DEBUG74 reason%s op%08x;CLOSE_ONLY_NO_IMPLICIT_END_OR_RETRY\n",observationFailure?observationFailure:session->failure?session->failure:"unknown",session->operation);return 1;
 }
 for(unsigned position=0;position<3;position++)for(unsigned row=0;row<8;row++){
  printf("DEBUG74_RECORD position%u word%02u",position,row*8);
  for(unsigned col=0;col<8;col++)printf(" %08x",records[position*64+row*8+col]);
  putchar('\n');
 }
 printf("PASS_DEBUG74 uploaded_parts%u pages%llu spans%u records3 clean_end1;DATA_CONSISTENCY_ONLY_EXTERNAL_NUMERICAL_COMPARISON_REQUIRED_NOT_TOKEN_RATE\n",
  session->partsUploaded,(unsigned long long)session->pagesUploaded,session->uploadSpans);
 return 0;
}
