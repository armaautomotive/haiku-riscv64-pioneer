/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "PioneerQ38ProjectionValidationSession.h"
struct ProjectionNativeValidationTransport {
 int Open(){puts("DEBUG_GPU_OPEN_AFTER_ORIGINAL_EMBEDDING_RMS_ALL4_PROJECTION_REFERENCES");return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  printf("DEBUG_IOCTL_BEGIN op%08x bytes%zu\n",unsigned(op),bytes);int status=ioctl(fd,op,data,bytes);
  printf("DEBUG_IOCTL_END op%08x status%d errno%d\n",unsigned(op),status,status?errno:0);return !status;
 }
 int Close(int fd){return close(fd);}
};
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--validate-token0"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("DEBUG_FIXED_PROJECTION_RESULT_QUALIFICATION;ORIGINAL_REFERENCES_BEFORE_GPU_FD;NO_WORK_OR_INFERENCE_SPEED_CLAIM");
 PioneerQ38PublicModelSource source;
 if(!source.Open() || !source.Ready()){puts("STOP_FULL_PUBLIC_GGUF_SHA_REQUIRED_BEFORE_ACCELERATOR_OPEN");return 1;}
 puts("FULL_PUBLIC_GGUF_SHA_SAME_FILE_PASS_BEFORE_ACCELERATOR_OPEN");
 ProjectionNativeValidationTransport io;PioneerQ38ProjectionValidationSession<ProjectionNativeValidationTransport> session(io,source);
 bool okay=session.Validate(0,0);if(!okay)session.Abort();bool clean=okay && session.Finish();
 printf("%s_DEBUG_NATIVE_QPS1 token0 four_projections16480_values complete_fixed_span18432_words actual_GDDR_readback_bitexact_to_independent_original_GGUF_references clean_end%u initialization_execution_readback_restore_us%llu;NO_TOKEN_RATE\n",
  okay && clean?"PASS":"FAIL",unsigned(clean),(unsigned long long)session.result.elapsed_us);
 return okay && clean?0:1;
}
