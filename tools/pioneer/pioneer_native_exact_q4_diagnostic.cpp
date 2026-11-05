/* SPDX-License-Identifier: MIT */
// Fixed synthetic arithmetic validation ONLY,never model/Work inference.
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeExactQ4Protocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeExactQ4Fixture.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
uint32_t Expected(unsigned word) {
 if(word>=32)return 0x7fc00001; // untouched output padding,NOT an accepted NaN
 const unsigned n=word%16;int sum=0;
 for(unsigned k=0;k<16;k++)sum+=int(PioneerNativeExactQ4Fixture::Quant(k,n))*PioneerNativeExactQ4Fixture::Input(k);
 const float value=float(sum);uint32_t bits;memcpy(&bits,&value,4);return bits;
} // independent fixed synthetic USER oracle;not kernel or learned inference
bool Result(const PioneerNativeExactQ4Request& r,uint64_t generation,unsigned state) {
 const auto& x=r.result;
 if(r.version!=1 || r.bits!=4 || r.expected_tag!=PioneerNativeExactQ4Tag || r.expected_generation!=generation || r.reserved
  || r.nonce!=1 || r.accept!=unsigned(state==PIONEER_NATIVE_BFP_CLOSED) || x.state!=state || x.cold
  || x.started!=1 || x.completed!=1 || !x.polls || x.polls>20000 || x.phase!=0x7000c || x.count!=2 || x.marker!=0x31433451)return false;
 for(unsigned c:x.controls)if(c)return false;
 for(unsigned pc:x.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
 const unsigned closed=state==PIONEER_NATIVE_BFP_CLOSED;
 return x.held==closed && x.l1_restored==closed && x.tlb_restored==closed;
}
void Dump(const PioneerNativeExactQ4Request& r) {
 for(unsigned word=0;word<64;word++)fprintf(stderr,"EXACT_Q4_RETURNED_WORD index%u got%08x expected%08x\n",word,r.result.output[word],Expected(word));
}
}
int main(int argc,char** argv) {
 if(argc!=2 || strcmp(argv[1],"--fixed-exact-q4")) {
  fputs("Use --fixed-exact-q4 for ONE isolated synthetic original-Q4 arithmetic qualification;NO model inference.\n",stderr);return 2;
 }
 if(geteuid()!=0)return 1;
 int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
 tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeExactQ4Tag;
 if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.driver!=55 || metadata.build_tag!=PioneerNativeExactQ4Tag
  || !metadata.generation || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1){close(fd);return 1;}
 PioneerNativeExactQ4Request request={};request.version=1;request.expected_tag=PioneerNativeExactQ4Tag;request.expected_generation=metadata.generation;
 if(ioctl(fd,PIONEER_NATIVE_EXACT_Q4_SUPPORT,&request,sizeof(request)) || !PioneerNativeBfpEmptyResult(request.result)){close(fd);return 1;}
 request={};request.version=1;request.bits=4;request.expected_tag=PioneerNativeExactQ4Tag;request.expected_generation=metadata.generation;
 if(ioctl(fd,PIONEER_NATIVE_EXACT_Q4_RUN,&request,sizeof(request)) || !Result(request,metadata.generation,PIONEER_NATIVE_BFP_RESULT)) {
  const auto& r=request.result;
  fprintf(stderr,"EXACT_Q4_RUN_UNPROVEN state%u cold%u started%u completed%u polls%u pc%x/%x phase%x count%u marker%x;CLOSE_ONLY_NO_END_OR_RETRY\n",
   r.state,r.cold,r.started,r.completed,r.polls,r.pc[0],r.pc[1],r.phase,r.count,r.marker);close(fd);return 1;
 }
 for(unsigned word=0;word<64;word++)if(request.result.output[word]!=Expected(word)) {
  fprintf(stderr,"EXACT_Q4_NUMERICAL_MISMATCH index%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",word,request.result.output[word],Expected(word));
  Dump(request);close(fd);return 1;
 }
 PioneerNativeExactQ4Request end={};end.version=1;end.bits=4;end.expected_tag=PioneerNativeExactQ4Tag;end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
 if(ioctl(fd,PIONEER_NATIVE_EXACT_Q4_END,&end,sizeof(end)) || !Result(end,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
  || memcmp(end.result.output,request.result.output,sizeof(request.result.output))) {
  fputs("EXACT_Q4_END_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
 }
 if(close(fd))return 1;
 printf("PASS_NATIVE_EXACT_Q4_FIXED_KERNEL rows16 dual_jobs2 exact_words32 padding_words32 polls%u generation%llu;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULLK_AFFINE_MODEL_OR_SPEED_CLAIM\n",request.result.polls,(unsigned long long)metadata.generation);return 0;
}
