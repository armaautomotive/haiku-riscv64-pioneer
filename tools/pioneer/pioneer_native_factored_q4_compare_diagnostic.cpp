/* SPDX-License-Identifier: MIT */
// Fixed CPU-reference DIAGNOSTIC,never a Work backend or CPU fallback.
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "PioneerNativeCompactQ4StreamSource.h"
#include "PioneerQ38Q4KernelTiming.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#if !defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE) || PIONEER_NATIVE_FACTORED_Q4_COMPARE!=1 || !defined(PIONEER_NATIVE_FACTORED_Q4_EXPECTED_GENERATION) || PIONEER_NATIVE_FACTORED_Q4_EXPECTED_GENERATION<2
#error Exact comparison111/fresh generation client required
#endif
#ifndef PIONEER_NATIVE_FACTORED_Q4_CLIENT_ROLE
#define PIONEER_NATIVE_FACTORED_Q4_CLIENT_ROLE 111
#endif
#if (PIONEER_NATIVE_FACTORED_Q4_CLIENT_ROLE!=111 && PIONEER_NATIVE_FACTORED_Q4_CLIENT_ROLE!=113) || ((PIONEER_NATIVE_FACTORED_Q4_CLIENT_ROLE==113)!=defined(PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST))
#error Exact comparison111/113 client role and manual-Last selection required
#endif
namespace {
using V=PioneerNativeCompactQ4StreamVariant;using M=PioneerNativeCompactQ4StreamMetadata;using R=PioneerNativeCompactQ4StreamRequest;
constexpr uint64_t Generation=PIONEER_NATIVE_FACTORED_Q4_EXPECTED_GENERATION;
bool Delivered(){return !fflush(stdout) && !ferror(stdout);}
bool Result(const R& r,unsigned sample,bool closed){
 const auto& c=r.result.compute;const unsigned restored=unsigned(closed);
 if(r.version!=1 || r.sample!=sample || r.expected_tag!=V::Tag || r.expected_generation!=Generation || r.nonce!=1 || r.accept!=restored || r.reserved
  || c.state!=(closed?PIONEER_NATIVE_BFP_CLOSED:PIONEER_NATIVE_BFP_RESULT) || c.cold || c.started!=1 || c.completed!=1 || !c.polls || c.polls>20000
  || c.phase!=0x7000c || c.count!=V::Count(sample) || c.marker!=V::Marker || (c.controls[0]&1) || c.controls[1] || c.controls[2])return false;
 for(unsigned pc:c.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
 return c.held==restored && c.l1_restored==restored && c.tlb_restored==restored && r.result.phase==(closed?10U:5U)
  && r.result.gddr_restored==restored && r.result.niu_restored==restored && r.result.clock_restored==restored
  && r.result.saved_l1==37136 && r.result.saved_gddr==(V::Width(sample)/256)*1280
  && r.result.requests==2*(V::Width(sample)/256)*26 && r.result.flits==2*(V::Width(sample)/256)*80;
}
void Timing(const R& r,unsigned sample){
 for(unsigned kind=0;kind<2;kind++){
  const auto* p=r.result.timing+kind*8;const unsigned state=PioneerQ38Q4KernelTiming::State(p,V::Width(sample),kind);
  if(state==1){
   const uint64_t start=uint64_t(p[4])|(uint64_t(p[5])<<32),end=uint64_t(p[6])|(uint64_t(p[7])<<32);
   printf("FACTORED_COMPARE_TIMING sample%u width%u kind%u state1 start%llu end%llu cycles%llu;0OLD_BF16_1NEW_FACTORED_NOT_MODEL_TPS\n",sample,V::Width(sample),kind,(unsigned long long)start,(unsigned long long)end,(unsigned long long)(end-start));
  }else printf("FACTORED_COMPARE_TIMING sample%u width%u kind%u state%u;UNAVAILABLE_NOT_TIMING_OR_CLEANUP_AUTHORITY\n",sample,V::Width(sample),kind,state);
 }
}
}
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--fixed-old-new-q4")){fprintf(stderr,"Use --fixed-old-new-q4 for isolated comparison%u,NOT Work or full-model inference.\n",V::Role);return 2;}
 if(geteuid()!=0)return 1;
 PioneerNativeCompactQ4StreamSource source;std::string error;
 puts("FACTORED_COMPARE_AUTHENTICATING_ORIGINAL_AND_BOTH_CPU_REFERENCES;NO_DEVICE_OPEN");if(!Delivered())return 1;
 if(!source.Prepare("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",error) || !source.Ready()){
  fprintf(stderr,"FACTORED_COMPARE_SOURCE_REFUSED %s;NO_DEVICE_OPEN\n",error.c_str());return 1;
 }
 puts("FACTORED_COMPARE_AUTHENTICATED c600de03 all9_originalPayloads_bothArithmeticReferences;CPU_DIAGNOSTIC_NOT_WORK");if(!Delivered())return 1;
 for(unsigned selected=0;selected<2;selected++){
  const unsigned sample=selected?6:0;const auto* old=M::Golden(sample);const auto* fresh=source.FactoredGolden(sample);
  if(!old || !fresh || !source.Ready())return 1;
  const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
  tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=V::Tag;
  if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1 || metadata.expected_tag || metadata.expected_generation
   || metadata.reserved[0] || metadata.reserved[1] || metadata.reserved[2] || metadata.driver!=55 || metadata.build_tag!=V::Tag || metadata.generation!=Generation
   || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1){
   fputs("FACTORED_COMPARE_METADATA_REFUSED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;
  }
  R support={};support.version=1;support.sample=PioneerNativeCompactQ4StreamNone;support.expected_tag=V::Tag;support.expected_generation=Generation;
  if(ioctl(fd,PIONEER_NATIVE_COMPACT_Q4_STREAM_SUPPORT,&support,sizeof(support)) || !PioneerNativeCompactQ4StreamRequestValid(support)
   || support.sample!=PioneerNativeCompactQ4StreamNone || support.nonce || support.accept || support.expected_generation!=Generation || support.result.compute.state!=PIONEER_NATIVE_BFP_EMPTY || support.result.compute.cold || !source.Ready()){
   fputs("FACTORED_COMPARE_SUPPORT_REFUSED;NO_RUN\n",stderr);close(fd);return 1;
  }
  R r={};r.version=1;r.sample=sample;r.expected_tag=V::Tag;r.expected_generation=Generation;
  if(ioctl(fd,PIONEER_NATIVE_COMPACT_Q4_STREAM_RUN,&r,sizeof(r)) || !Result(r,sample,false) || !source.Ready()){
   const auto& c=r.result.compute;
   fprintf(stderr,"FACTORED_COMPARE_RUN_UNPROVEN sample%u state%u cold%u started%u completed%u polls%u pc%x/%x phase%x count%x marker%x journal%u saved_l1%u;CLOSE_ONLY_NO_END_RETRY\n",sample,c.state,c.cold,c.started,c.completed,c.polls,c.pc[0],c.pc[1],c.phase,c.count,c.marker,r.result.phase,r.result.saved_l1);
   close(fd);return 1;
  }
#ifdef PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST
  // Completed DATA is captured in full BEFORE deciding, END or accepting
  // timing. Unlike111's first-mismatch loop this preserves every returned
  // value on failure. Exact comparisons/ownership rules remain unchanged.
  unsigned mismatches=0;
  for(unsigned word=0;word<64;word++){
   const unsigned expected=word<32?old[word]:fresh[word-32],actual=r.result.compute.output[word];
   const bool exact=actual==expected;if(!exact)mismatches++;
   printf("FACTORED_COMPARE_DATA_WORD sample%u kind%u row%u got%08x expected%08x exact%u\n",sample,word/32,word%32,actual,expected,unsigned(exact));
  }
  printf("FACTORED_COMPARE_DATA_DONE sample%u words64 mismatches%u;DATA_ONLY_NO_END_OR_SPEED_AUTHORITY\n",sample,mismatches);
  if(!Delivered() || mismatches || !source.Ready()){
   fprintf(stderr,"FACTORED_COMPARE_REJECTED sample%u mismatches%u;CLOSE_ONLY_NO_END_RESET_RETRY\n",sample,mismatches);close(fd);return 1;
  }
#else
  for(unsigned word=0;word<64;word++){
   const unsigned expected=word<32?old[word]:fresh[word-32];
   if(r.result.compute.output[word]!=expected){
    fprintf(stderr,"FACTORED_COMPARE_MISMATCH sample%u kind%u row%u got%08x expected%08x;CLOSE_ONLY_NO_END_RETRY\n",sample,word/32,word%32,r.result.compute.output[word],expected);close(fd);return 1;
   }
  }
#endif
  if(!source.Ready()){close(fd);return 1;}
  R end={};end.version=1;end.sample=sample;end.expected_tag=V::Tag;end.expected_generation=Generation;end.nonce=1;end.accept=1;
  if(ioctl(fd,PIONEER_NATIVE_COMPACT_Q4_STREAM_END,&end,sizeof(end)) || !Result(end,sample,true)
   || memcmp(end.result.compute.output,r.result.compute.output,sizeof(r.result.compute.output))
   || memcmp(end.result.compute.controls,r.result.compute.controls,sizeof(r.result.compute.controls))
   || memcmp(end.result.timing,r.result.timing,sizeof(r.result.timing)) || end.result.compute.polls!=r.result.compute.polls || !source.Ready()){
   fputs("FACTORED_COMPARE_END_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
  }
  if(close(fd))return 1;
  for(unsigned word=0;word<64;word++)printf("FACTORED_COMPARE_WORD sample%u kind%u row%u got%08x expected%08x\n",sample,word/32,word%32,r.result.compute.output[word],word<32?old[word]:fresh[word-32]);
  printf("PASS_NATIVE_FACTORED_COMPARE sample%u width%u exact64 generation%llu saved_l137136 saved_gddr%u requests%u flits%u;EXPLICIT_END_L1_GDDR_NIU_TLB_FIFO_CLOCK_PCI_RESTORED_NOT_MODEL_TPS\n",sample,V::Width(sample),(unsigned long long)Generation,r.result.saved_gddr,r.result.requests,r.result.flits);
  Timing(r,sample);if(!Delivered())return 1;
 }
 puts("PASS_NATIVE_FACTORED_COMPARE_ALL samples2 exact128 cleanEnds2;DIAGNOSTIC_NOT_WORK_NOT_MODEL_QUALITY_OR_TPS");return Delivered()?0:1;
}
