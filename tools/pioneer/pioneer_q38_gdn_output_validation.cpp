/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "PioneerQ38GdnOutputValidationSession.h"
struct GdnOutputNativeValidationTransport {
 int Open(){puts("DEBUG_GPU_OPEN_AFTER_ORIGINAL_GDN_REFERENCES;OUTPUT_REFERENCE_ONLY_AFTER_RETURNED_GDN_PASSES");return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  printf("DEBUG_IOCTL_BEGIN op%08x bytes%zu\n",unsigned(op),bytes);int status=ioctl(fd,op,data,bytes);
  printf("DEBUG_IOCTL_END op%08x status%d errno%d\n",unsigned(op),status,status?errno:0);return !status;
 }
 int Close(int fd){return close(fd);}
};
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--validate-token0"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("DEBUG55_FIXED_FIRST_TOKEN_LAYER0_QGT1_TO_QOP1;CPU_VALIDATION_SEPARATE_FROM_WORK;NO_INFERENCE_OR_TOKEN_RATE_CLAIM");
 PioneerQ38PublicModelSource source;
 if(!source.Open() || !source.Ready()){puts("STOP_FULL_PUBLIC_GGUF_SHA_REQUIRED_BEFORE_ACCELERATOR_OPEN");return 1;}
 puts("FULL_PUBLIC_GGUF_SHA_SAME_FILE_PASS_BEFORE_ACCELERATOR_OPEN");
 GdnOutputNativeValidationTransport io;PioneerQ38GdnOutputValidationSession<GdnOutputNativeValidationTransport> session(io,source);
 bool okay=session.Validate(0);if(!okay)session.Abort();bool clean=okay && session.Finish();
 if(!okay || !clean){
  const auto& d=session.diagnostics;const char* section=d.word==UINT32_MAX?"contract_or_transport":
   d.reason==decltype(session)::Failure::ReplyMetadata?"reply_header":d.word<829632?"gdn":d.word<832704?"native_operands":d.word<832896?"scales":"ssm_out";
  printf("DEBUG_CPU_ONLY_FIRST_FAILURE reason%s op%08x section%s word%u expected_bits%08x actual_bits%08x reply_captured%u;NO_ADDITIONAL_DEVICE_IO\n",
   session.FailureName(d.reason),d.op,section,d.word,unsigned(d.expected),unsigned(d.actual),unsigned(d.reply_captured));
  if(d.reply_captured)for(unsigned row=0;row<8;row++){
   printf("DEBUG_CPU_REPLY_HEADER word%02u",row*8);
   for(unsigned i=0;i<8;i++)printf(" %08x",unsigned(d.reply_header[row*8+i]));
   putchar('\n');
  }
 }
 printf("%s_DEBUG_NATIVE_QGT1_QOP1 token0 layer0 all829632_GDN_values_STRICT_original_bounds_then_validated_ACTUAL_gated6144 all3072_operands_192scales_5120rows_BIT_EXACT clean_end%u init_execution_readback_restore_us%llu;NO_TOKEN_RATE\n",
  okay && clean?"PASS":"FAIL",unsigned(clean),(unsigned long long)session.result.elapsed_us);
 return okay && clean?0:1;
}
