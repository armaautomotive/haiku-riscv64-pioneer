/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "PioneerQ38ResidentLayerValidationSession.h"
struct ResidentLayerNativeValidationTransport {
 int Open(){puts("DEBUG69_GPU_OPEN_AFTER_SAME_FILE_FULL_SHA_ORIGINAL_REFERENCES_AND_SEAL");return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  printf("DEBUG69_IOCTL_BEGIN op%08x bytes%zu\n",unsigned(op),bytes);errno=0;int status=ioctl(fd,op,data,bytes);
  printf("DEBUG69_IOCTL_END op%08x status%d errno%d\n",unsigned(op),status,status?errno:0);return !status;
 }
 int Close(int fd){return close(fd);}
};
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--validate-token0"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("DEBUG69_FIXED_TOKEN0_POSITION0_LAYER0_COMPLETE_CONV_GDN_OUTPUT_NORM_FFN_FINAL;CPU_OBSERVER_SEPARATE;NOT_WORK_OR_FULL_MODEL_INFERENCE");
 auto source=OpenResidentLayerOriginalSource();
 if(!source){puts("STOP_FULL_SAME_FILE_PUBLIC_GGUF_SHA_REQUIRED_BEFORE_ACCELERATOR_OPEN");return 1;}
 ResidentLayerNativeValidationTransport io;
 PioneerQ38ResidentLayerValidationSession<ResidentLayerNativeValidationTransport> session(io,*source);
 bool okay=session.Validate();if(!okay)session.Abort();bool clean=okay && session.Finish();
 if(!okay || !clean){
  const auto& d=session.diagnostics;
  printf("DEBUG69_CPU_ONLY_FAILURE reason%s op%08x word%u expected%08x actual%08x reply_captured%u;NO_ADDITIONAL_DEVICE_IO\n",
   session.FailureName(d.reason),d.op,d.word,unsigned(d.expected),unsigned(d.actual),unsigned(d.reply_captured));
  if(d.reply_captured)for(unsigned row=0;row<8;row++){
   printf("DEBUG69_CPU_REPLY_HEADER word%02u",row*8);
   for(unsigned i=0;i<8;i++)printf(" %08x",unsigned(d.reply_header[row*8+i]));
   putchar('\n');
  }
 }
 printf("DEBUG69_CPU_OBSERVERS ideal_libm_strict_passed%u final_bits%u max_abs%.12g relative_l2%.12g;instruction_policy_passed%u final_bits%u max_abs%.12g;DISTINCT_REPORTS_NOT_WORK_OR_RATE\n",
  session.idealComparison.passed,session.idealComparison.finalDifferences,session.idealComparison.finalMaxAbsolute,session.idealComparison.finalRelativeL2,
  session.instructionComparison.passed,session.instructionComparison.finalDifferences,session.instructionComparison.finalMaxAbsolute);
 printf("%s_DEBUG69 token0 layer0 original_upload_pages%u spans%u fixed_readback_words1630208 clean_end%u execution_readback_restore_us%llu;NOT_FULL_MODEL_OR_TOKEN_RATE\n",
  okay && clean?"PASS":"FAIL",session.uploaded_pages,session.upload_spans,unsigned(clean),(unsigned long long)session.result.elapsed_us);
 return okay && clean?0:1;
}
