/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "PioneerQ38EmbeddingSession.h"
struct EmbeddingNativeTransport {
 int Open(){return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){return !ioctl(fd,op,data,bytes);}
 int Close(int fd){return close(fd);}
};
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--produce-token0"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);PioneerQ38PublicModelSource source;
 puts("EMBEDDING_ARITHMETIC_QUALIFICATION_ONLY;NO_CPU_REFERENCE_OR_MODEL_INFERENCE_OR_SPEED_CLAIM");
 if(!source.Open() || !source.Ready()){puts("STOP_FULL_PUBLIC_GGUF_SHA_REQUIRED_BEFORE_ACCELERATOR_OPEN");return 1;}
 puts("FULL_PUBLIC_GGUF_SHA_SAME_FILE_PASS_BEFORE_ACCELERATOR_OPEN");
 EmbeddingNativeTransport io;PioneerQ38EmbeddingSession<EmbeddingNativeTransport> session(io,source);
 bool okay=session.Produce(0,0);
 if(!okay)session.Abort();
 bool clean=okay && session.Finish();
 printf("%s_NATIVE_EMBEDDING token0 values5120 resident_GDDR input_ready%u clean_end%u initialization_execution_restore_us%llu;NO_NUMERIC_REFERENCE_OR_TOKEN_RATE\n",
  okay && clean?"PASS":"FAIL",session.result.input_ready,unsigned(clean),(unsigned long long)session.result.elapsed_us);
 return okay && clean?0:1;
}
