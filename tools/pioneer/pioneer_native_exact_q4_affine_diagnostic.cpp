/* SPDX-License-Identifier: MIT */
// Fixed synthetic arithmetic validation ONLY. NEVER a model/Work mode.
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeExactQ4AffineProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
uint32_t Expected(unsigned word){
 if(word>=32)return 0x7fc00001;
 const unsigned seed=word<16?37:176,n=word%16;float old=0;
 // Independent ORIGINAL q/input/metadata reference,not the shader's bit
 // construction or prepared source. No learned model is opened here.
 for(unsigned p=0;p<17408/16;p++){
  int sum=0,dot=0;
  for(unsigned k=0;k<16;k++){const int x=int((seed*29+p*13+k*37)%256)-128;sum+=x;dot+=int((seed+p*11+k*17+n*3)%16)*x;}
  float coefficient=float((seed+(p/2)*3+n)%64)/1024.0f;if((seed+p/2+n)&1)coefficient=-coefficient;
  const float minimum=-float((seed*7+p/2+n*3)%64)/4096.0f;
  const uint32_t raw=0x3b800000U|(((seed+p/16)*7919)&0x7fffff)|(((seed+p/16)&1)?0x80000000U:0);
  float scale;memcpy(&scale,&raw,4);
  const float product=(coefficient*scale)*float(dot);const float correction=(minimum*scale)*float(sum);
  old=old+(product+correction);
 }
 uint32_t result;memcpy(&result,&old,4);return result;
}
bool Result(const PioneerNativeExactQ4AffineRequest& r,uint64_t generation,unsigned state){
 const auto& x=r.result;
 if(r.version!=1 || r.bits!=4 || r.expected_tag!=PioneerNativeExactQ4AffineTag || r.expected_generation!=generation || r.reserved
  || r.nonce!=1 || r.accept!=unsigned(state==PIONEER_NATIVE_BFP_CLOSED) || x.state!=state || x.cold
  || x.started!=1 || x.completed!=1 || !x.polls || x.polls>20000 || x.phase!=0x7000c || x.count!=2 || x.marker!=0x31413451)return false;
 for(unsigned c:x.controls)if(c)return false;
 for(unsigned pc:x.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
 const unsigned closed=state==PIONEER_NATIVE_BFP_CLOSED;
 return x.held==closed && x.l1_restored==closed && x.tlb_restored==closed;
}
void Dump(const PioneerNativeExactQ4AffineRequest& r){
 for(unsigned word=0;word<64;word++)fprintf(stderr,"EXACT_Q4_AFFINE_RETURNED_WORD index%u got%08x expected%08x\n",word,r.result.output[word],Expected(word));
}
}
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--fixed-exact-q4-full-k"))return 2;
 if(geteuid()!=0)return 1;
 const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
 tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeExactQ4AffineTag;
 if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1 || metadata.driver!=55
  || metadata.build_tag!=PioneerNativeExactQ4AffineTag || !metadata.generation || metadata.quiesced || metadata.cold || metadata.active_owner
  || metadata.open_count!=1 || metadata.guard_version!=1){close(fd);return 1;}
 PioneerNativeExactQ4AffineRequest support={};support.version=1;support.expected_tag=PioneerNativeExactQ4AffineTag;support.expected_generation=metadata.generation;
 if(ioctl(fd,PIONEER_NATIVE_EXACT_Q4_AFFINE_SUPPORT,&support,sizeof(support)) || support.result.state!=PIONEER_NATIVE_BFP_EMPTY || support.result.cold){close(fd);return 1;}
 PioneerNativeExactQ4AffineRequest request={};request.version=1;request.bits=4;request.expected_tag=PioneerNativeExactQ4AffineTag;request.expected_generation=metadata.generation;
 if(ioctl(fd,PIONEER_NATIVE_EXACT_Q4_AFFINE_RUN,&request,sizeof(request)) || !Result(request,metadata.generation,PIONEER_NATIVE_BFP_RESULT)){
  fprintf(stderr,"EXACT_Q4_AFFINE_RUN_UNPROVEN;CLOSE_ONLY_NO_END_OR_RETRY\n");close(fd);return 1;
 }
 for(unsigned word=0;word<64;word++)if(request.result.output[word]!=Expected(word)){
  fprintf(stderr,"EXACT_Q4_AFFINE_NUMERICAL_MISMATCH index%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",word,request.result.output[word],Expected(word));
  Dump(request);close(fd);return 1;
 }
 PioneerNativeExactQ4AffineRequest end={};end.version=1;end.bits=4;end.expected_tag=PioneerNativeExactQ4AffineTag;end.expected_generation=metadata.generation;end.nonce=end.accept=1;
 if(ioctl(fd,PIONEER_NATIVE_EXACT_Q4_AFFINE_END,&end,sizeof(end)) || !Result(end,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
  || memcmp(request.result.output,end.result.output,sizeof(end.result.output))){close(fd);return 1;}
 if(close(fd))return 1;
 printf("PASS_NATIVE_EXACT_Q4_AFFINE_FIXED_KERNEL k17408 rows16 dual_jobs2 exact_words32 padding_words32 polls%u generation%llu;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_OR_SPEED_CLAIM\n",request.result.polls,(unsigned long long)metadata.generation);return 0;
}
