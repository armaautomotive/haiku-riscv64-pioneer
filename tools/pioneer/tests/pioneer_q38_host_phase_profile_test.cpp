/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HostPhaseProfile.h"
static unsigned lines=0;
template<class... A> static void TestPrint(const char* format,A... values) {
 char line[1024];assert(snprintf(line,sizeof(line),format,values...)>0);lines++;
 assert(strstr(line,"P100A_Q38_HOST_PHASE"));
}
#define dprintf TestPrint
struct Transport {
 PioneerQ38HostPhaseProfile q38HostPhaseProfile;
 bool q38HostPhaseReportAllowed=false;
#define PIONEER_Q38_HOST_PHASE_PROFILE
#include "host_phase_add.inc"
#include "host_phase_report.inc"
};
#undef dprintf
struct Context {bool bankStorageOnly=false;Transport io;};
struct Result {bool host_released=false,cold=false;};
static void Publication(Context* context,bool clean,const Result& result) {
#include "host_phase_publication.inc"
}
int main() {
 unsigned cases=0;
 for(unsigned type=0;type<3;type++)for(unsigned k=0;k<3;k++) {
  PioneerQ38HostPhaseProfile p;uint64_t times[8];
  for(unsigned i=0;i<8;i++)times[i]=UINT64_C(0x100000000)+i;
  unsigned packed=type==0?8:type==1?12:14,inputs=k==0?5120:k==1?6144:17408;
  for(unsigned sample=0;sample<1001;sample++)p.Add(packed,inputs,119,true,times);
  const auto& b=p.At(type,k);assert(b.valid && b.count==1001 && !p.Ignored());
  for(unsigned i=0;i<8;i++)assert(b.first[i]==times[i] && b.warm[i]==times[i]*1000);
  p.Add(packed,inputs,2,true,times);assert(p.Ignored()==1 && b.count==1001);
  p.Add(0,inputs,119,true,times);p.Add(packed,0,119,true,times);assert(p.Ignored()==3);
  p.Add(packed,inputs,119,false,times);assert(!b.valid && b.count==1001);
  p.Add(packed,inputs,119,true,times);assert(!b.valid && b.count==1001);cases++;
 }
 for(unsigned word=0;word<8;word++) {
  PioneerQ38HostPhaseProfile p;uint64_t times[8]={};
  p.Add(12,17408,119,true,times);times[word]=UINT64_MAX;
  p.Add(12,17408,119,true,times);auto before=p.At(1,2);times[word]=1;
  p.Add(12,17408,119,true,times);const auto& b=p.At(1,2);
  assert(!b.valid && b.count==2 && !memcmp(b.warm,before.warm,sizeof(b.warm)));
  p.Add(12,17408,119,true,times);assert(!b.valid && b.count==2);cases++;
 }
 {PioneerQ38HostPhaseProfile p;p.Add(12,17408,119,true,nullptr);assert(!p.At(1,2).valid && !p.At(1,2).count);cases++;}
 // ACTUAL report+publication guard:only clean/host-released/not-cold,
 // bank session;formatting iterates CPU buckets only,never device/clock.
 for(unsigned mask=0;mask<32;mask++) {
  Context c;c.bankStorageOnly=mask&1;Result r;r.host_released=mask&2;r.cold=mask&4;
  c.io.q38HostPhaseReportAllowed=mask&16;
  uint64_t times[8]={};c.io.Q38DispatchProfile(12,17408,119,true,times);
  c.io.Q38DispatchProfile(12,17408,119,true,times);
  lines=0;Publication(&c,mask&8,r);assert(lines==(mask==27?2U:0U));cases++;
 }
 printf("PASS %u host-phase profile cases:9shapes/1000warm64-bit sums,first excluded,ignored roles,8 atomic overflow/sticky failures,NULL refusal;ACTUAL CPU-only report and32 explicit-END-publication guards;NO clock/card/control authority\n",cases);
}
