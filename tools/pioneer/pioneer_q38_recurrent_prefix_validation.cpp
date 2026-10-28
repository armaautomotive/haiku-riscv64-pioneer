/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include "PioneerQ38RecurrentPrefixValidationSession.h"
struct PrefixNativeValidationTransport {
 int Open(){
  puts("DEBUG73_GPU_OPEN_AFTER_FULL_C600_SHA_PRIVATE_3LAYER_CPU_REFERENCE_169RAW_EXTENTS_AND_SEAL");
  return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);
 }
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  printf("DEBUG73_IOCTL_BEGIN op%08x bytes%zu\n",unsigned(op),bytes);errno=0;int status=ioctl(fd,op,data,bytes);
  printf("DEBUG73_IOCTL_END op%08x status%d errno%d\n",unsigned(op),status,status?errno:0);return !status;
 }
 int Close(int fd){return close(fd);}
};
int main(int argc,char** argv){
 if(argc!=2 || std::strcmp(argv[1],"--validate-token0-prefix0-2"))return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("DEBUG73_ORIGINAL_TOKEN0_EMBEDDING_RECURRENT_LAYERS0_2_ONLY_NOT_FULL_MODEL_WORK_OR_TOKEN_RATE");
 auto source=OpenPioneerQ38RecurrentPrefixValidationSource();if(!source)return 1;
 PrefixNativeValidationTransport io;
 auto session=std::make_unique<PioneerQ38RecurrentPrefixValidationSession<PrefixNativeValidationTransport>>(io,*source);
 bool okay=session->Validate();bool clean=okay && session->Finish();
 if(!okay || !clean){
  printf("DEBUG73_FAILURE reason%s op%08x captured%u;CPU_METADATA_ONLY_NO_ADDITIONAL_DEVICE_IO\n",
   session->failure?session->failure:"unknown",session->operation,unsigned(session->reply_captured));
  if(session->reply_captured)for(unsigned row=0;row<12;row++){
   printf("DEBUG73_REPLY_HEADER word%02u",row*8);for(unsigned i=0;i<8;i++)printf(" %08x",session->reply_header[row*8+i]);putchar('\n');
  }
 }
 printf("%s_DEBUG73 original_pages%u uploads%u checked%u differences%u nonfinite%u max_abs%.12g relative_l2%.12g clean_end%u diagnostic_us%llu;NOT_TOKEN_RATE\n",
  okay && clean?"PASS":"FAIL",session->uploaded_pages,session->upload_spans,session->comparison.checked,session->comparison.differences,
  session->comparison.nonfinite,session->comparison.maxAbsolute,session->comparison.relativeL2,unsigned(clean),(unsigned long long)session->result.elapsed_us);
 return okay && clean?0:1;
}
