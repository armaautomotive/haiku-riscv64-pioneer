/* SPDX-License-Identifier: MIT */
// Explicit fixed, preverified head-band DEBUG only. Never Assistant/Work.
#if !defined(PIONEER_QGW8_HEAD_BAND_130) || PIONEER_QGW8_HEAD_BAND_130 != 1 \
 || !defined(PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64) || PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64 != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Exact isolated QGW8 head-band diagnostic required
#endif
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeDramModelBandProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#define PIONEER_QGW8_HEAD_SOURCE_DIAGNOSTIC 1
#include "PioneerQgw8HeadBandSource.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
using Meta=PioneerNativeBfpModelBandMetadata;
using Variant=PioneerNativeDramModelBandVariant;
constexpr const char* Original="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";
bool Result(const PioneerNativeDramModelBandRequest& r,unsigned sample,uint64_t generation,unsigned state){
 const auto& c=r.result.compute;const unsigned restored=unsigned(state==PIONEER_NATIVE_BFP_CLOSED);
 if(r.version!=1 || r.sample!=sample || r.expected_tag!=PioneerNativeDramModelBandTag || r.expected_generation!=generation
  || r.nonce!=1 || r.reserved || r.accept!=restored || c.state!=state || c.cold || c.started!=1 || c.completed!=1
  || !c.polls || c.polls>20000 || c.phase!=0x7000c || c.count!=Variant::Count || c.marker!=Variant::Marker
  || (c.controls[0]&1) || c.controls[1] || c.controls[2])return false;
 for(unsigned pc:c.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
 return c.held==restored && c.l1_restored==restored && c.tlb_restored==restored
  && r.result.phase==(restored?10U:5U) && r.result.gddr_restored==restored && r.result.niu_restored==restored
  && r.result.clock_restored==restored && r.result.requests==320 && r.result.flits==2880
  && r.result.saved_l1==PioneerNativeDramLoadVariant::L1Words && r.result.saved_gddr==46080;
}
bool Delivered(){return !fflush(stdout) && !ferror(stdout);}
}
int main(int argc,char** argv){
 if(argc!=2 || (strcmp(argv[1],"--fixed-qgw8-head-bands") && strcmp(argv[1],"--verify-source-only"))){
  fputs("Use --verify-source-only or --fixed-qgw8-head-bands for DEBUG, not model/Work inference.\n",stderr);return 2;
 }
 if(geteuid()!=0)return 1;
 PioneerQgw8HeadBandSource source;std::string error;
 puts("HEAD130_AUTHENTICATING_ORIGINAL_REFERENCE_AND_PAYLOAD;CPU_ONLY_NO_DEVICE_YET");if(!Delivered())return 1;
 if(!source.Prepare(Original,error) || !source.Ready()){
  fprintf(stderr,"HEAD130_SOURCE_REFUSED: %s;NO_DEVICE_OPEN\n",error.c_str());return 1;
 }
 puts("HEAD130_SOURCE_VERIFIED real_head_bands3 fixed_cpu_reference;DEBUG_NOT_WORK");if(!Delivered())return 1;
 if(!strcmp(argv[1],"--verify-source-only")){
  puts("PASS_HEAD130_SOURCE_ONLY;NO_DEVICE_OPEN");return Delivered()?0:1;
 }
 uint64_t seriesGeneration=0;
 for(unsigned sample=0;sample<3;sample++){
  if(!source.Ready())return 1;
  const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
  tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeDramModelBandTag;
  if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1 || metadata.expected_tag || metadata.expected_generation
   || metadata.reserved[0] || metadata.reserved[1] || metadata.reserved[2] || metadata.driver!=55 || metadata.build_tag!=PioneerNativeDramModelBandTag
   || !metadata.generation || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1){
   fputs("HEAD130_METADATA_REFUSED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;
  }
  if(seriesGeneration && metadata.generation!=seriesGeneration){fputs("HEAD130_EPOCH_CHANGED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;}
  seriesGeneration=metadata.generation;
  PioneerNativeDramModelBandRequest r={};r.version=1;r.sample=PioneerNativeDramModelBandNone;r.expected_tag=PioneerNativeDramModelBandTag;r.expected_generation=metadata.generation;
  if(ioctl(fd,PIONEER_NATIVE_DRAM_MODEL_BAND_SUPPORT,&r,sizeof(r)) || !PioneerNativeDramModelBandRequestValid(r)
   || r.sample!=PioneerNativeDramModelBandNone || r.nonce || r.accept || r.expected_generation!=metadata.generation || !source.Ready()){
   fputs("HEAD130_SUPPORT_SOURCE_REFUSED;NO_RUN\n",stderr);close(fd);return 1;
  }
  r={};r.version=1;r.sample=sample;r.expected_tag=PioneerNativeDramModelBandTag;r.expected_generation=metadata.generation;
  if(ioctl(fd,PIONEER_NATIVE_DRAM_MODEL_BAND_RUN,&r,sizeof(r)) || !Result(r,sample,metadata.generation,PIONEER_NATIVE_BFP_RESULT) || !source.Ready()){
   fputs("HEAD130_RUN_UNPROVEN;CLOSE_ONLY_NO_END_OR_RETRY\n",stderr);close(fd);return 1;
  }
  const auto* golden=Meta::Golden(sample);if(!golden){close(fd);return 1;}
  for(unsigned word=0;word<64;word++)if(r.result.compute.output[word]!=golden[word%32]){
   fprintf(stderr,"HEAD130_NUMERICAL_MISMATCH sample%u row%u pass%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",
    sample,word%32,word/32,r.result.compute.output[word],golden[word%32]);close(fd);return 1;
  }
  if(!source.Ready()){fputs("HEAD130_SOURCE_CHANGED;CLOSE_ONLY_NO_END\n",stderr);close(fd);return 1;}
  PioneerNativeDramModelBandRequest end={};end.version=1;end.sample=sample;end.expected_tag=PioneerNativeDramModelBandTag;
  end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
  if(ioctl(fd,PIONEER_NATIVE_DRAM_MODEL_BAND_END,&end,sizeof(end)) || !Result(end,sample,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
   || memcmp(end.result.compute.output,r.result.compute.output,sizeof(r.result.compute.output))
   || memcmp(end.result.compute.controls,r.result.compute.controls,sizeof(r.result.compute.controls))
   || end.result.compute.polls!=r.result.compute.polls || !source.Ready()){
   fputs("HEAD130_END_SOURCE_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
  }
  if(close(fd))return 1;
  printf("HEAD130_SAMPLE sample%u first%u exact_words64 clean_end1 generation%llu;DEBUG_NOT_WORK\n",
   sample,Meta::First(sample),(unsigned long long)metadata.generation);
  if(!Delivered())return 1;
 }
 puts("PASS_HEAD130 three real-head bands exact192 clean_end3;NOT_FULL_MODEL_QUALITY_OR_SPEED");
 return Delivered()?0:1;
}
