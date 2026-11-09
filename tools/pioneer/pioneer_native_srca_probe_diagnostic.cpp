/* SPDX-License-Identifier: MIT */
// One fixed synthetic SrcA qualification only. Not model/Work inference.
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeSrcaProbeProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
uint32_t Expected(unsigned word){
 const unsigned lane=word%32;
 const float value=word<32?((lane&15)<4?0.f:float(lane-3)/256):float(lane+1)/256;
 uint32_t bits;memcpy(&bits,&value,4);return bits;
} // Host-only oracle for fixed fixture; no inference validation callback.
bool Result(const PioneerNativeSrcaProbeRequest& r,uint64_t generation,unsigned state){
 const auto& x=r.result;
 if(r.version!=1 || r.bits!=4 || r.expected_tag!=PioneerNativeSrcaProbeTag || r.expected_generation!=generation || r.reserved
  || r.nonce!=1 || r.accept!=unsigned(state==PIONEER_NATIVE_BFP_CLOSED) || x.state!=state || x.cold
  || x.started!=1 || x.completed!=1 || !x.polls || x.polls>20000 || x.phase!=0x7000c || x.count!=2 || x.marker!=0x31504353)return false;
 for(unsigned c:x.controls)if(c)return false;
 for(unsigned pc:x.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
 const unsigned closed=state==PIONEER_NATIVE_BFP_CLOSED;
 return x.held==closed && x.l1_restored==closed && x.tlb_restored==closed;
}
void Dump(const PioneerNativeSrcaProbeRequest& r){
 for(unsigned word=0;word<64;word++)fprintf(stderr,"SRCA_PROBE_RETURNED_WORD index%u got%08x expected%08x\n",word,r.result.output[word],Expected(word));
}
}
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--fixed-srca-probe")){
  fputs("Use --fixed-srca-probe for ONE isolated synthetic two-base PV qualification; NO model inference.\n",stderr);return 2;
 }
 if(geteuid()!=0)return 1;
 int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
 tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeSrcaProbeTag;
 if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.driver!=55 || metadata.build_tag!=PioneerNativeSrcaProbeTag
  || !metadata.generation || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1){close(fd);return 1;}
 PioneerNativeSrcaProbeRequest request={};request.version=1;request.expected_tag=PioneerNativeSrcaProbeTag;request.expected_generation=metadata.generation;
 if(ioctl(fd,PIONEER_NATIVE_SRCA_PROBE_SUPPORT,&request,sizeof(request)) || !PioneerNativeBfpEmptyResult(request.result)){close(fd);return 1;}
 request={};request.version=1;request.bits=4;request.expected_tag=PioneerNativeSrcaProbeTag;request.expected_generation=metadata.generation;
 if(ioctl(fd,PIONEER_NATIVE_SRCA_PROBE_RUN,&request,sizeof(request)) || !Result(request,metadata.generation,PIONEER_NATIVE_BFP_RESULT)){
  const auto& r=request.result;
  fprintf(stderr,"SRCA_PROBE_RUN_UNPROVEN state%u cold%u started%u completed%u polls%u pc%x/%x phase%x count%u marker%x;CLOSE_ONLY_NO_END_OR_RETRY\n",
   r.state,r.cold,r.started,r.completed,r.polls,r.pc[0],r.pc[1],r.phase,r.count,r.marker);close(fd);return 1;
 }
 for(unsigned word=0;word<64;word++)if(request.result.output[word]!=Expected(word)){
  fprintf(stderr,"SRCA_PROBE_NUMERICAL_MISMATCH index%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",word,request.result.output[word],Expected(word));
  Dump(request);close(fd);return 1;
 }
 PioneerNativeSrcaProbeRequest end={};end.version=1;end.bits=4;end.expected_tag=PioneerNativeSrcaProbeTag;end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
 if(ioctl(fd,PIONEER_NATIVE_SRCA_PROBE_END,&end,sizeof(end)) || !Result(end,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
  || memcmp(end.result.output,request.result.output,sizeof(request.result.output))){
  fputs("SRCA_PROBE_END_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
 }
 if(close(fd))return 1;
 printf("PASS_NATIVE_SRCA_PROBE_FIXED old_shift32 corrected32 polls%u generation%llu;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_OR_SPEED_CLAIM\n",request.result.polls,(unsigned long long)metadata.generation);return 0;
}
