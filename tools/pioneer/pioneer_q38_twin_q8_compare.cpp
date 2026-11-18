/* SPDX-License-Identifier: MIT */
// Fixed on-card projection comparison. CPU compares outputs after clean
// END only; never supplies a result, computes a projection or falls back.
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TwinQ8DiagnosticProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
extern "C" {
#include "../../src/libs/libsolv/solv/sha2.h"
}

namespace D=PioneerQ38TwinQ8Diagnostic;
namespace S=D::S;

static bool Ready(int fd,uint64_t gen){
 tt_pioneer_driver_reload r={};r.version=1;r.expected_tag=D::Tag;r.expected_generation=gen;
 return !ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&r,sizeof(r)) && r.build_tag==D::Tag && r.generation==gen
  && r.guard_version==1 && !r.quiesced && !r.cold && !r.active_owner && r.open_count==1;
}
static void Failure(const D::Request& r){
 const auto& x=r.result;
 fprintf(stderr,"Q8_PAIR_FAIL arm=%u variant=%u outer=%u item=%u pages=%u upload=%u/%u/%u/%u "
  "boot=%08x/%08x/%08x stop_pc=%08x journal=%u/%u/%08x/%08x/%08x "
  "sequence=%u/%u/%08x/%08x/%08x polls=%u cold=%u close=%u/%u/%u/%u/%u;CLOSE_ONLY_NO_RETRY\n",
  r.arm,r.variant,unsigned(r.step),r.item,r.uploaded,r.uploadState,r.uploadCompleted,r.uploadMismatches,r.uploadRecovery,
  r.bootFault,r.bootStatus,r.bootPC,r.stopPC,r.journalOp,r.journalWord,r.journalAddress,r.journalSeen,r.journalExpected,
  unsigned(x.stage),unsigned(x.error),x.at,x.seen,x.expected,x.polls,r.cold,
  r.closed.state,r.closed.active,r.closed.pci_restored,r.closed.host_released,r.closed.cold);
}
static bool Run(int fd,uint64_t gen,S::Arm arm,unsigned variant,D::Request& r){
 if(!Ready(fd,gen)){fprintf(stderr,"Q8_PAIR_METADATA_REFUSED;CLOSE_ONLY\n");return false;}
 r={};r.tag=D::Tag;r.generation=gen;r.version=D::Version;r.arm=unsigned(arm);r.variant=variant;
 if(ioctl(fd,D::RunOperation,&r,sizeof(r))){
  fprintf(stderr,"Q8_PAIR_IOCTL_FAILED errno=%d;CLOSE_ONLY\n",errno);return false;
 }
 const auto& x=r.result;
 if(r.tag!=D::Tag || r.generation!=gen || r.version!=D::Version || r.arm!=unsigned(arm) || r.variant!=variant
  || r.reserved || r.cold || r.step!=D::Step::Complete || r.uploaded!=S::WeightPages
  || x.stage!=S::Stage::Complete || x.error!=S::Error::None || x.unsafe || !x.closed || !x.stopped || !x.matrixMicros
  || !D::Closed(r.closed)){
  Failure(r);return false;
 }
 return true;
}
static bool Verify(const D::Request& r){
 static const char* const expected[]={
  "0e94ceba949d9266e0383ca43b96c73da4bcaf0785fec9d45b4570f73e021254",
  "42f5a264959edb0b327fc337a227ae2f4bcb55bc74a637cbde09087b057ebfd2"};
 if(r.variant>=2)return false;
 uint8_t bytes[S::Rows*4]={};
 for(unsigned row=0;row<S::Rows;row++)for(unsigned b=0;b<4;b++)
  bytes[row*4+b]=uint8_t(r.result.output[row]>>(8*b));
 char actual[SHA256_DIGEST_STRING_LENGTH]={};
 if(!solv_SHA256_Data(bytes,sizeof(bytes),actual) || strcmp(actual,expected[r.variant])){
  fprintf(stderr,"Q8_PAIR_EXPECTED_OUTPUT_MISMATCH arm=%u variant=%u shader_sha=%s;SESSION_CLOSED_NO_RETRY\n",
   r.arm,r.variant,actual);
  return false;
 }
 printf("Q8_PAIR_CLOSED arm=%u variant=%u pages=%u polls=%u matrix_wait_us=%llu output_sha=%s\n",r.arm,r.variant,
        r.uploaded,r.result.polls,(unsigned long long)r.result.matrixMicros,actual);
 return true;
}
int main(int argc,char** argv){
 if(argc!=3 || strcmp(argv[1],"--compare") || strspn(argv[2],"0123456789")!=strlen(argv[2]))return 2;
 char* end=nullptr;errno=0;
 const uint64_t gen=strtoull(argv[2],&end,10);
 if(errno || !gen || !end || *end)return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 const int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);
 if(fd<0){fprintf(stderr,"Q8_PAIR_OPEN_FAILED errno=%d\n",errno);return 1;}
 for(unsigned variant=0;variant<2;variant++){
  D::Request raw,native;
  if(!Run(fd,gen,S::Arm::Raw,variant,raw) || !Verify(raw)
   || !Run(fd,gen,S::Arm::Native,variant,native) || !Verify(native)){close(fd);return 1;}
  if(memcmp(raw.result.output,native.result.output,sizeof(raw.result.output))){
   fprintf(stderr,"Q8_PAIR_VALUE_MISMATCH variant=%u;BOTH_EXPLICIT_ENDS_CLOSED_NO_RETRY\n",variant);close(fd);return 1;
  }
 }
 if(!Ready(fd,gen)){close(fd);return 1;}
 if(close(fd))return 1;
 puts("PASS_Q8_PAIR_FOUR_CLOSED_MATCHING_PINNED_ARMS;NO_TOKENS_PER_SECOND");
 return 0;
}
