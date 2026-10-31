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
 uint64_t reported=0;bool cancelDiagnostics=false;
 int Open(){
  puts("DEBUG74_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT");
  return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);
 }
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  const bool token=cancelDiagnostics && op==TT_PCI_PROBE_Q38_TOKEN_VALIDATE;
  if(token)printf("DEBUG74_CANCEL_IOCTL_BEGIN operation%u\n",static_cast<PioneerQ38TokenControl::Packet*>(data)->request.operation);
  errno=0;const int status=ioctl(fd,op,data,bytes);
  if(status)printf("DEBUG74_IOCTL_FAILED op%08x status%d errno%d;NO_RETRY\n",unsigned(op),status,errno);
  if(token){
   const auto& r=static_cast<PioneerQ38TokenControl::Packet*>(data)->reply;
   printf("DEBUG74_CANCEL_UNTRUSTED_REPLY status%d version%u operation%u started%u ready%u unsafe%u complete%u stage%u\n",
    status,r.version,r.operation,r.started,r.ready,r.unsafe,r.complete,r.stage);
  } // logging ONLY;the session independently validates every accepted reply
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
 const bool cancelTrial=!std::strcmp(argv[1],"--validate-cancel-boundary");
 if(!preflight && !cancelTrial && std::strcmp(argv[1],"--validate-three-positions"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts(cancelTrial?"DEBUG74_CANCELLATION_LIFECYCLE_DIAGNOSTIC_NOT_FULL_MODEL_WORK_OR_TOKEN_RATE"
  :"DEBUG74_FIXED_2PROMPT_3POSITION_DIAGNOSTIC_NOT_ASSISTANT_WORK_OR_TOKEN_RATE");
 PioneerQ38PublicModelSource source;TokenValidationTransport io;io.cancelDiagnostics=cancelTrial;
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
 if(okay && cancelTrial)okay=session->RequestCancellation(); // explicit healthy trial,NOT error recovery
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
 uint32_t records[3*64]={};if(okay && !cancelTrial)okay=session->CopyRecords(records,3*64);
 if(!okay){
  session->Abort();printf("FAIL_DEBUG74 reason%s op%08x;CLOSE_ONLY_NO_IMPLICIT_END_OR_RETRY\n",observationFailure?observationFailure:session->failure?session->failure:"unknown",session->operation);return 1;
 }
 if(cancelTrial){
  const bool consumed=session->CancellationConsumed();
  printf("%s_DEBUG74_CANCEL requested1 cancelled%u committed%u layers%u clean_end1;CANCELLATION_LIFECYCLE_ONLY_NOT_FULL_MODEL_WORK_OR_TOKEN_RATE\n",
   consumed?"PASS":"MISSED_WINDOW",unsigned(consumed),session->CommittedRecords(),session->CancelledLayers());
  return consumed?0:2;
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
