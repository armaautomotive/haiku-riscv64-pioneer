/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "PioneerQ38EmbeddingSession.h"
struct EmbeddingNativeTransport {
 int Open(){
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
  puts("DEBUG_GPU_OPEN_AFTER_SAME_FILE_REFERENCE_PREPARATION");
#endif
  return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
  printf("DEBUG_IOCTL_BEGIN op%08x bytes%zu\n",unsigned(op),bytes);
#endif
  int status=ioctl(fd,op,data,bytes);
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
  printf("DEBUG_IOCTL_END op%08x status%d errno%d\n",unsigned(op),status,status?errno:0);
#endif
  return !status;}
 int Close(int fd){return close(fd);}
};
int main(int argc,char** argv){
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
 if(argc!=2 || strcmp(argv[1],"--validate-token0"))return 2;
#else
 if(argc!=2 || strcmp(argv[1],"--produce-token0"))return 2;
#endif
 setvbuf(stdout,nullptr,_IOLBF,0);PioneerQ38PublicModelSource source;
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
 puts("DEBUG_EMBEDDING_NUMERIC_VALIDATION;INDEPENDENT_CPU_REFERENCE_BEFORE_GPU_FD;NO_WORK_OR_INFERENCE_SPEED_CLAIM");
#else
 puts("EMBEDDING_ARITHMETIC_QUALIFICATION_ONLY;NO_CPU_REFERENCE_OR_MODEL_INFERENCE_OR_SPEED_CLAIM");
#endif
 if(!source.Open() || !source.Ready()){puts("STOP_FULL_PUBLIC_GGUF_SHA_REQUIRED_BEFORE_ACCELERATOR_OPEN");return 1;}
 puts("FULL_PUBLIC_GGUF_SHA_SAME_FILE_PASS_BEFORE_ACCELERATOR_OPEN");
 EmbeddingNativeTransport io;PioneerQ38EmbeddingSession<EmbeddingNativeTransport> session(io,source);
 bool okay=session.Produce(0,0);
 if(!okay)session.Abort();
 bool clean=okay && session.Finish();
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT
 printf("%s_DEBUG_NATIVE_EMBEDDING token0 values5120 actual_GDDR_readback_bitexact_to_independent_original_GGUF_reference clean_end%u initialization_execution_readback_restore_us%llu;NO_TOKEN_RATE\n",
  okay && clean?"PASS":"FAIL",unsigned(clean),(unsigned long long)session.result.elapsed_us);
#else
 printf("%s_NATIVE_EMBEDDING token0 values5120 resident_GDDR input_ready%u clean_end%u initialization_execution_restore_us%llu;NO_NUMERIC_REFERENCE_OR_TOKEN_RATE\n",
  okay && clean?"PASS":"FAIL",session.result.input_ready,unsigned(clean),(unsigned long long)session.result.elapsed_us);
#endif
 return okay && clean?0:1;
}
