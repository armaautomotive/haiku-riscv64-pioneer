/* SPDX-License-Identifier: MIT */
// Captures the actual debug client with fake source and ioctl endpoints.
// Nothing here opens the P100A or exercises a kernel driver.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "PioneerNativeBfpModelBandMetadata.h"
#define PIONEER_NATIVE_MODEL_BAND_SOURCE_H
static unsigned scenario=0,opened=0,closed=0,queries=0,supports=0,runs=0,ends=0,prepared=0,ready=0;
static unsigned failedWord=0;
struct FakeSource {
 bool Prepare(const char* path,std::string& error){assert(!prepared++ && !opened && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf"));error="fake";return scenario!=1;}
 bool Ready(){ready++;return !(scenario==2 && ready==1) && !(scenario==3 && supports==1 && !runs)
  && !(scenario==4 && runs==1 && !ends) && !(scenario==5 && ends==1);}
};
static uid_t FakeEuid();static int FakeOpen(const char*,int);static int FakeClose(int);static int FakeIoctl(int,unsigned long,void*,size_t);
#define PioneerNativeModelBandSource FakeSource
#define geteuid FakeEuid
#define open FakeOpen
#define close FakeClose
#define ioctl FakeIoctl
#define main CapturedPairMain
#include "../pioneer_native_model_band_pair_diagnostic.cpp"
#undef main
#undef ioctl
#undef close
#undef open
#undef geteuid
#undef PioneerNativeModelBandSource
static uid_t FakeEuid(){return scenario==6?1:0;}
static int FakeOpen(const char* path,int flags){assert(!opened++ && !strcmp(path,"/dev/misc/tenstorrent_pci_probe") && flags==(O_RDWR|O_CLOEXEC));return 123;}
static int FakeClose(int fd){assert(fd==123 && !closed++);return scenario==7?-1:0;}
static void Fill(PioneerNativeModelBandPairRequest& r,bool end){
 assert(PioneerNativeModelBandPairRequestValid(r) && !r.sample && r.expected_generation==19);
 assert(end?(r.nonce==1 && r.accept==1):(!r.nonce && !r.accept));r.nonce=1;r.overlap_verified=1;
 for(unsigned i=0;i<2;i++){auto& v=r.result[i];v.state=end?PIONEER_NATIVE_BFP_CLOSED:PIONEER_NATIVE_BFP_RESULT;
  v.started=v.completed=1;v.polls=1;v.pc[0]=0x70038;v.pc[1]=0x7003c;v.phase=0x7000c;
  v.count=0x04000000U|PioneerNativeBfpModelBandMetadata::Width;v.marker=0x3342464e;
  v.held=v.l1_restored=v.tlb_restored=end;
  for(unsigned word=0;word<64;word++)v.output[word]=PioneerNativeBfpModelBandMetadata::Golden(i)[word%32];
 }
}
static int FakeIoctl(int fd,unsigned long op,void* data,size_t bytes){
 assert(fd==123 && opened==1 && !closed);
 if(op==TT_PCI_PROBE_RELOAD_STATUS){
  assert(bytes==sizeof(tt_pioneer_driver_reload) && !queries++);
  auto& m=*static_cast<tt_pioneer_driver_reload*>(data);assert(m.version==1 && m.expected_tag==PioneerNativeModelBandPairTag);
  m={};m.version=1;m.driver=55;m.build_tag=PioneerNativeModelBandPairTag;m.generation=19;m.open_count=1;m.guard_version=1;
  if(scenario==8)m.build_tag^=1;if(scenario==9)m.cold=1;return scenario==10?-1:0;
 }
 assert(bytes==sizeof(PioneerNativeModelBandPairRequest));auto& r=*static_cast<PioneerNativeModelBandPairRequest*>(data);
 if(op==PIONEER_NATIVE_MODEL_BAND_PAIR_SUPPORT){
  assert(queries==1 && !supports++ && !runs && !ends && PioneerNativeModelBandPairRequestValid(r) && r.sample==UINT32_MAX);
  if(scenario==11)r.result[1].cold=1;return scenario==12?-1:0;
 }
 if(op==PIONEER_NATIVE_MODEL_BAND_PAIR_RUN){
  assert(supports==1 && !runs++ && !ends);Fill(r,false);
  if(scenario>=20 && scenario<40){unsigned target=(scenario-20)/10,kind=(scenario-20)%10;auto& v=r.result[target];
   if(kind==0)v.output[failedWord]^=1;if(kind==1)v.output[63]^=1;if(kind==2)v.state=0;if(kind==3)v.cold=1;
   if(kind==4)v.started=0;if(kind==5)v.completed=0;if(kind==6)v.marker^=1;if(kind==7)v.pc[0]=0;
   if(kind==8)v.polls=0;if(kind==9)v.count^=1;
  }
  return scenario==13?-1:0;
 }
 assert(op==PIONEER_NATIVE_MODEL_BAND_PAIR_END && runs==1 && !ends++);Fill(r,true);
 if(scenario>=40 && scenario<50){unsigned target=(scenario-40)/5,kind=(scenario-40)%5;auto& v=r.result[target];
  if(kind==0)v.output[0]^=1;if(kind==1)v.held=0;if(kind==2)v.l1_restored=0;if(kind==3)v.tlb_restored=0;if(kind==4)v.cold=1;
 }
 return scenario==14?-1:0;
}
int main(){
 char name[]="pair",option[]="--fixed-qkv-pair";char* args[]={name,option,nullptr};
 for(scenario=0;scenario<50;scenario++){
  if((scenario>14 && scenario<20))continue;
  opened=closed=queries=supports=runs=ends=prepared=ready=0;
  int code=CapturedPairMain(2,args);assert(code==(scenario==0?0:1));
  if(scenario==1 || scenario==2 || scenario==6){assert(!opened && !closed && !queries && !runs && !ends);continue;}
  assert(opened==1 && closed==1 && queries==1);
  if(scenario>=8 && scenario<=10){assert(!supports && !runs && !ends);continue;}
  assert(supports==1);
  if(scenario==3 || scenario==11 || scenario==12){assert(!runs && !ends);continue;}
  assert(runs==1);
  if(scenario==4 || scenario==13 || (scenario>=20 && scenario<40)){assert(!ends);continue;}
  assert(ends==1);
 }
 for(unsigned target=0;target<2;target++)for(failedWord=0;failedWord<64;failedWord++){
  scenario=20+target*10;opened=closed=queries=supports=runs=ends=prepared=ready=0;
  assert(CapturedPairMain(2,args)==1);
  assert(opened==1 && closed==1 && queries==1 && supports==1 && runs==1 && !ends);
 }
 puts("PASS_PAIR_ACTUAL_CLIENT fake_ioctl_source_both_cores_failure_injection NO_DEVICE NO_WORK_VALIDATION");
}
