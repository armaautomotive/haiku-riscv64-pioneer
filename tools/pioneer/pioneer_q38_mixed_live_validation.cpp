/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include "PioneerQ38MixedLiveValidationSession.h"
struct DenseNativeValidationTransport {
 int Open(){puts("DEBUG70_GPU_OPEN_AFTER_FULL_SHA_ORIGINAL_REFERENCES_AND_SEAL");return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  printf("DEBUG70_IOCTL_BEGIN op%08x bytes%zu\n",unsigned(op),bytes);errno=0;int status=ioctl(fd,op,data,bytes);
  printf("DEBUG70_IOCTL_END op%08x status%d errno%d\n",unsigned(op),status,status?errno:0);return !status;
 }
 int Close(int fd){return close(fd);}
};
int main(int argc,char** argv){
 if(argc!=2 || std::strcmp(argv[1],"--validate-token0-dense3"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("DEBUG70_STANDALONE_DENSE_LAYER3_FROM_TOKEN0_EMBEDDING_NOT_VALID_FULL_MODEL_PREFIX_OR_WORK");
 auto source=OpenPioneerQ38MixedLiveOriginalSource();if(!source)return 1;
 DenseNativeValidationTransport io;PioneerQ38MixedLiveValidationSession<DenseNativeValidationTransport> session(io,*source);
 bool okay=session.Validate();bool clean=okay && session.Finish();
 if(!okay || !clean){
  printf("DEBUG70_FAILURE reason%s op%08x captured%u;CPU_METADATA_ONLY_NO_ADDITIONAL_DEVICE_IO\n",session.failure?session.failure:"unknown",session.operation,unsigned(session.reply_captured));
  if(session.reply_captured)for(unsigned row=0;row<8;row++){
   printf("DEBUG70_REPLY_HEADER word%02u",row*8);for(unsigned i=0;i<8;i++)printf(" %08x",session.reply_header[row*8+i]);putchar('\n');
  }
 }
 printf("%s_DEBUG70 original_pages%u spans%u checked%u differences%u nonfinite%u max_abs%.12g relative_l2%.12g clean_end%u diagnostic_us%llu;NOT_TOKEN_RATE\n",
  okay && clean?"PASS":"FAIL",session.uploaded_pages,session.upload_spans,session.comparison.checked,session.comparison.differences,
  session.comparison.nonfinite,session.comparison.maxAbsolute,session.comparison.relativeL2,unsigned(clean),(unsigned long long)session.result.elapsed_us);
 return okay && clean?0:1;
}
