/* SPDX-License-Identifier: MIT */
// Fixed QKV debug reference; never Work, never opens the card on source failure.
#if !defined(PIONEER_NATIVE_MODEL_BAND_PAIR_DIAGNOSTIC) || PIONEER_NATIVE_MODEL_BAND_PAIR_DIAGNOSTIC != 1 \
 || !defined(PIONEER_NATIVE_RECURRENT_QKV_BAND_SOURCE) || PIONEER_NATIVE_RECURRENT_QKV_BAND_SOURCE != 1
#error Pair diagnostic requires its own protocol and the authenticated QKV source
#endif
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeModelBandPairProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "PioneerNativeModelBandSource.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
using Meta=PioneerNativeBfpModelBandMetadata;
bool Result(const PioneerNativeModelBandPairRequest& r,uint64_t generation,unsigned state){
 if(r.version!=1 || r.sample || r.reserved2 || r.expected_tag!=PioneerNativeModelBandPairTag
  || r.expected_generation!=generation || r.nonce!=1 || r.accept!=unsigned(state==PIONEER_NATIVE_BFP_CLOSED))return false;
 for(const auto& v:r.result){
  if(v.state!=state || v.cold || v.started!=1 || v.completed!=1 || !v.polls || v.polls>20000
   || v.phase!=0x7000c || v.count!=(0x04000000U|Meta::Width) || v.marker!=0x3342464e)return false;
  for(unsigned pc:v.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
  const unsigned restored=unsigned(state==PIONEER_NATIVE_BFP_CLOSED);
  if(v.held!=restored || v.l1_restored!=restored || v.tlb_restored!=restored)return false;
 }
 return true;
}
bool Delivered(){return !fflush(stdout) && !ferror(stdout);}
}
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--fixed-qkv-pair")) {fputs("Use --fixed-qkv-pair for two fixed DEBUG bands,NOT Work inference.\n",stderr);return 2;}
 if(geteuid()!=0)return 1;
 PioneerNativeModelBandSource source;std::string error;
 puts("AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET");if(!Delivered())return 1;
 if(!source.Prepare("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",error) || !source.Ready()){
  fprintf(stderr,"PAIR_SOURCE_REFUSED: %s;NO_DEVICE_OPEN\n",error.c_str());return 1;}
 int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
 tt_pioneer_driver_reload m={};m.version=1;m.expected_tag=PioneerNativeModelBandPairTag;
 if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&m,sizeof(m)) || m.version!=1 || m.expected_tag || m.expected_generation
  || m.reserved[0] || m.reserved[1] || m.reserved[2] || m.driver!=55 || m.build_tag!=PioneerNativeModelBandPairTag
  || !m.generation || m.quiesced || m.cold || m.active_owner || m.open_count!=1 || m.guard_version!=1){
  fputs("PAIR_METADATA_REFUSED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;}
 PioneerNativeModelBandPairRequest r={};r.version=1;r.sample=UINT32_MAX;r.expected_tag=PioneerNativeModelBandPairTag;r.expected_generation=m.generation;
 if(ioctl(fd,PIONEER_NATIVE_MODEL_BAND_PAIR_SUPPORT,&r,sizeof(r)) || !PioneerNativeModelBandPairRequestValid(r)
  || r.sample!=UINT32_MAX || r.nonce || r.accept || r.expected_generation!=m.generation || !source.Ready()){
  fputs("PAIR_SUPPORT_SOURCE_REFUSED;NO_RUN\n",stderr);close(fd);return 1;}
 r={};r.version=1;r.expected_tag=PioneerNativeModelBandPairTag;r.expected_generation=m.generation;
 if(ioctl(fd,PIONEER_NATIVE_MODEL_BAND_PAIR_RUN,&r,sizeof(r)) || !Result(r,m.generation,PIONEER_NATIVE_BFP_RESULT) || !source.Ready()){
  fputs("PAIR_RUN_UNPROVEN;CLOSE_ONLY_NO_END_OR_RETRY\n",stderr);close(fd);return 1;}
 for(unsigned i=0;i<2;i++){
  const auto* golden=Meta::Golden(i);if(!golden){close(fd);return 1;}
  for(unsigned n=0;n<64;n++)if(r.result[i].output[n]!=golden[n%32]){
   fprintf(stderr,"PAIR_NUMERICAL_MISMATCH sample%u word%u;CLOSE_ONLY_NO_END_OR_RETRY\n",i,n);close(fd);return 1;}
 }
 if(!source.Ready()){fputs("PAIR_SOURCE_CHANGED;CLOSE_ONLY_NO_END\n",stderr);close(fd);return 1;}
 PioneerNativeModelBandPairRequest end={};end.version=1;end.expected_tag=PioneerNativeModelBandPairTag;end.expected_generation=m.generation;end.nonce=1;end.accept=1;
 if(ioctl(fd,PIONEER_NATIVE_MODEL_BAND_PAIR_END,&end,sizeof(end)) || !Result(end,m.generation,PIONEER_NATIVE_BFP_CLOSED)
  || memcmp(end.result[0].output,r.result[0].output,sizeof(r.result[0].output))
  || memcmp(end.result[1].output,r.result[1].output,sizeof(r.result[1].output)) || !source.Ready()){
  fputs("PAIR_END_SOURCE_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;}
 if(close(fd))return 1;
 for(unsigned i=0;i<2;i++)for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)
  printf("PAIR_QKV_WORD sample%u first%u pass%u row%u got%08x\n",i,Meta::First(i),pass,row,r.result[i].output[pass*32+row]);
 printf("PASS_NATIVE_QKV_PAIR samples2 exact_words128 generation%llu;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_OVERLAP_OR_INFERENCE_SPEED_CLAIM\n",(unsigned long long)m.generation);
 return Delivered()?0:1;
}
