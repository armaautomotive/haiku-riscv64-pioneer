/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <OS.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include "PioneerQ38TokenBoundaryValidationSession.h"
#include "PioneerQ38TokenBoundaryValidationCLI.h"
#include "PioneerQ38TokenBoundaryDelay.h"
#include "PioneerQ38TokenBoundaryAbort.h"
static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"fixed raw snapshot byte order");
struct BoundaryValidationTransport {
 uint64_t reported=0;
 int Open(){puts("DEBUG75_BOUNDARY_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT");
  return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  errno=0;const int status=ioctl(fd,op,data,bytes);const int saved=errno;
  if(status)printf("DEBUG75_BOUNDARY_IOCTL_FAILED op%08x status%d errno%d;NO_RETRY\n",unsigned(op),status,saved);
  return status==0;
 }
 int Close(int fd){return close(fd);}
 void Progress(uint64_t bytes,uint64_t total){
  if(bytes==total || bytes-reported>=UINT64_C(64)*1024*1024){
   printf("DEBUG75_BOUNDARY_UPLOADED %llu/%llu\n",(unsigned long long)bytes,(unsigned long long)total);reported=bytes;
  }
 }
};
static bool WriteSnapshotExclusive(const char* path,const PioneerQ38TokenBoundarySnapshot& data){
 const int fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
 if(fd<0)return false;
 const auto* bytes=reinterpret_cast<const unsigned char*>(&data);size_t used=0;bool okay=true;
 while(used<sizeof(data)){
  const ssize_t count=write(fd,bytes+used,sizeof(data)-used);
  if(count<0 && errno==EINTR)continue;
  if(count<=0){okay=false;break;}
  used+=size_t(count);
 }
 if(okay && fsync(fd))okay=false;
 if(close(fd))okay=false;
 return okay; // never overwrite/unlink;on failure leave any new partial file for diagnosis
}
struct BoundaryValidationClock {
 int64_t Now(){return system_time();}
 bool Sleep(int64_t duration){return snooze(duration)==B_OK;}
};
int main(int argc,char** argv){
 PioneerQ38TokenBoundaryValidationOptions options;
 if(!PioneerQ38TokenBoundaryValidationArguments(argc,argv,options))return 2;
 if(options.output){
  struct stat st={};errno=0;
  if(!lstat(options.output,&st) || errno!=ENOENT)return 2;
 } // existing output/symlink/lookup errors refuse BEFORE model or device access
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("DEBUG75_BOUNDARY_OBSERVED_LAYER_BOUNDARY_DIAGNOSTIC_NOT_EXACT_LAYER_SELECTION_WORK_OR_TOKEN_RATE");
 PioneerQ38PublicModelSource source;BoundaryValidationTransport io;
 auto session=std::make_unique<PioneerQ38TokenBoundaryValidationSession<BoundaryValidationTransport>>(io,source);
 if(options.preflight){
  const bool okay=session->Preflight() && session->InspectLearnedPageSamples();
  printf("%s_DEBUG75_BOUNDARY_SOURCE_ONLY reason%s sampled_pages10653 planned_bytes%llu;FULL_SHA_PLUS_RAW_SAMPLES_NO_ACCELERATOR_OPEN_OR_INFERENCE\n",
   okay?"PASS":"FAIL",session->failure?session->failure:"none",(unsigned long long)session->bytesPlanned);
  return okay?0:1;
 }
 if(!session->Begin()){
  printf("FAIL_DEBUG75_BOUNDARY reason%s op%08x;CLOSE_ONLY_RESOURCES_MAY_BE_RETAINED\n",session->failure?session->failure:"begin",session->operation);return 1;
 }
 puts("DEBUG75_BOUNDARY_START_ACKNOWLEDGED;NOT_COMPLETED_LAYER_OR_TOKEN");
 // The delay is host intent,not a target-layer proof. Only genuine terminal
 // metadata determines which boundary may be exported and compared.
 BoundaryValidationClock clock;int64_t delayedUntil=0;
 if(!PioneerQ38TokenBoundaryDelay(clock,options.delayMs,delayedUntil)){
  session->Abort();puts("FAIL_DEBUG75_BOUNDARY_DELAY_OR_CLOCK;CLOSE_ONLY_NO_RETRY");return 1;
 }
 if(!session->RequestCancellation()){
  printf("FAIL_DEBUG75_BOUNDARY_CANCEL reason%s;CLOSE_ONLY_NO_RETRY\n",session->failure?session->failure:"cancel");return 1;
 }
 puts("DEBUG75_BOUNDARY_CANCEL_REQUEST_ACKNOWLEDGED;NOT_STOP_OR_END");
 const bigtime_t began=delayedUntil;bigtime_t previous=began;bool okay=began>=0;
 bool reportedPending=false;unsigned reportedWord=0;bigtime_t pendingReportedAt=began;
 const char* observationFailure=okay?nullptr:"clock";
 for(unsigned polls=0;okay && !session->TerminalReady();polls++){
  const bigtime_t now=system_time();
  if(now<previous || now-began>=INT64_C(1800000000) || polls>=18000){observationFailure="deadline_or_clock";okay=false;break;}
  previous=now;
  if(!session->Poll()){okay=false;break;}
  PioneerQ38TokenBoundaryValidationSession<BoundaryValidationTransport>::PendingObservation observation;
  if(session->CopyPendingObservation(observation)
   && (!reportedPending || observation.word!=reportedWord || now-pendingReportedAt>=INT64_C(10000000))){
   printf("DEBUG75_BOUNDARY_PENDING poll%u readout_word%u raw_frontends%08x/%08x poll_started_elapsed_us%lld;NOT_LAYER_PC_OR_COMPLETION_PROOF\n",
    polls,observation.word,observation.firstFrontend,observation.secondFrontend,(long long)(now-began));
   reportedPending=true;reportedWord=observation.word;pendingReportedAt=now;
  }
  if(!session->TerminalReady() && snooze(100000)!=B_OK){observationFailure="snooze";okay=false;break;}
 }
 if(okay){
  puts("DEBUG75_BOUNDARY_TERMINAL_READY_BEGIN_HOLD_DRAIN_READ_AND_RESTORE;NOT_CLEAN_END_YET");
  okay=session->Finish();
 }
 if(!okay){
  const char* reason=PioneerQ38TokenBoundaryAbortReason(*session,observationFailure);
  printf("FAIL_DEBUG75_BOUNDARY reason%s op%08x;CLOSE_ONLY_NO_RETRY_OR_IMPLICIT_END\n",reason,session->operation);return 1;
 }
 if(!session->Captured()){
  printf("FAIL_DEBUG75_BOUNDARY_WINDOW_MISSED clean_end1 commits%u cancelled_layers%u;NO_SNAPSHOT_NO_RETRY\n",
   session->CommittedRecords(),session->CancelledLayers());return 1;
 }
 auto snapshot=std::make_unique<PioneerQ38TokenBoundarySnapshot>();
 if(!session->CopySnapshot(snapshot.get(),sizeof(*snapshot)) || !WriteSnapshotExclusive(options.output,*snapshot)){
  puts("FAIL_DEBUG75_BOUNDARY_OUTPUT clean_end1;DO_NOT_USE_ANY_PARTIAL_FILE");return 1;
 }
 printf("PASS_DEBUG75_BOUNDARY_SNAPSHOT words5120 position0 observed_completed_layers%u requested_delay_ms%u clean_end1;DELAY_NOT_EXACT_LAYER_SELECTION_RAW_NOT_NUMERICAL_WORK_OR_TPS_PROOF\n",
  session->CancelledLayers(),options.delayMs);
 return 0;
}
