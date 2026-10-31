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
#include "PioneerQ38TokenHiddenValidationSession.h"
#include "PioneerQ38TokenHiddenValidationCLI.h"
static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"fixed raw snapshot byte order");
struct HiddenValidationTransport {
 uint64_t reported=0;
 int Open(){puts("DEBUG75_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT");
  return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  errno=0;const int status=ioctl(fd,op,data,bytes);const int saved=errno;
  if(status)printf("DEBUG75_IOCTL_FAILED op%08x status%d errno%d;NO_RETRY\n",unsigned(op),status,saved);
  return status==0;
 }
 int Close(int fd){return close(fd);}
 void Progress(uint64_t bytes,uint64_t total){
  if(bytes==total || bytes-reported>=UINT64_C(64)*1024*1024){
   printf("DEBUG75_UPLOADED %llu/%llu\n",(unsigned long long)bytes,(unsigned long long)total);reported=bytes;
  }
 }
};
static bool WriteSnapshotExclusive(const char* path,const PioneerQ38TokenHiddenSnapshot& data){
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
int main(int argc,char** argv){
 PioneerQ38TokenHiddenValidationOptions options;
 if(!PioneerQ38TokenHiddenValidationArguments(argc,argv,options))return 2;
 if(options.output){
  struct stat st={};errno=0;
  if(!lstat(options.output,&st) || errno!=ENOENT)return 2;
 } // existing output/symlink/lookup errors refuse BEFORE model or device access
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("DEBUG75_FIRST_LAYER_HIDDEN_DIAGNOSTIC_NOT_ASSISTANT_WORK_OR_TOKEN_RATE");
 PioneerQ38PublicModelSource source;HiddenValidationTransport io;
 auto session=std::make_unique<PioneerQ38TokenHiddenValidationSession<HiddenValidationTransport>>(io,source);
 if(options.preflight){
  const bool okay=session->Preflight() && session->InspectLearnedPageSamples();
  printf("%s_DEBUG75_SOURCE_ONLY reason%s sampled_pages10653 planned_bytes%llu;FULL_SHA_PLUS_RAW_SAMPLES_NO_ACCELERATOR_OPEN_OR_INFERENCE\n",
   okay?"PASS":"FAIL",session->failure?session->failure:"none",(unsigned long long)session->bytesPlanned);
  return okay?0:1;
 }
 if(!session->Begin() || !session->RequestCancellation()){
  printf("FAIL_DEBUG75 reason%s op%08x;CLOSE_ONLY_RESOURCES_MAY_BE_RETAINED\n",session->failure?session->failure:"begin_or_cancel",session->operation);return 1;
 }
 const bigtime_t began=system_time();bigtime_t previous=began;bool okay=began>=0;
 const char* observationFailure=okay?nullptr:"clock";
 for(unsigned polls=0;okay && !session->TerminalReady();polls++){
  const bigtime_t now=system_time();
  if(now<previous || now-began>=INT64_C(1800000000) || polls>=18000){observationFailure="deadline_or_clock";okay=false;break;}
  previous=now;
  if(!session->Poll()){okay=false;break;}
  if(!session->TerminalReady() && snooze(100000)!=B_OK){observationFailure="snooze";okay=false;break;}
 }
 if(okay)okay=session->Finish();
 if(!okay){
  session->Abort();printf("FAIL_DEBUG75 reason%s op%08x;CLOSE_ONLY_NO_RETRY_OR_IMPLICIT_END\n",
   session->failure?session->failure:observationFailure?observationFailure:"unknown",session->operation);return 1;
 }
 if(!session->Captured()){
  printf("FAIL_DEBUG75_FIRST_LAYER_WINDOW_MISSED clean_end1 commits%u cancelled_layers%u;NO_SNAPSHOT_NO_RETRY\n",
   session->CommittedRecords(),session->CancelledLayers());return 1;
 }
 auto snapshot=std::make_unique<PioneerQ38TokenHiddenSnapshot>();
 if(!session->CopySnapshot(snapshot.get(),sizeof(*snapshot)) || !WriteSnapshotExclusive(options.output,*snapshot)){
  puts("FAIL_DEBUG75_OUTPUT clean_end1;DO_NOT_USE_ANY_PARTIAL_FILE");return 1;
 }
 puts("PASS_DEBUG75_HIDDEN_SNAPSHOT words5120 position0 completed_layers1 clean_end1;RAW_DIAGNOSTIC_ONLY_NOT_NUMERICAL_WORK_OR_TPS_PROOF");
 return 0;
}
