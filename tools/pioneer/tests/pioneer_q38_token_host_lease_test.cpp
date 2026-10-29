/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenHostLease.h"
// OS endpoint/metadata DATA tests ONLY. Eligibility below is a simulated
// endpoint;it cannot stand in for actual native launch/STOP/PCI/END proof.
struct IO {
 PioneerQ38TokenViewLease views;PioneerQ38TokenHostLease hosts;
 tt_host_page_result input={};PioneerOutputBufferLease bulk;PioneerWideOutputLease wide;
 std::array<int32_t,29> device;std::array<int32_t,3> host={301,302,303};
 std::array<uintptr_t,3> pointer={0x10000000,0x20000000,0x30000000};
 std::array<uint64_t,3> physical={UINT64_C(0x120000000),UINT64_C(0x180000000),UINT64_C(0x140000000)};
 std::array<bool,3> attempted={},quarantine={};std::vector<unsigned> calls;
 bool eligible=true;unsigned fail=0,lose=0,mutate=0,deviceCalls=0;
 IO(){
  for(unsigned i=0;i<29;i++)device[i]=int32_t(200+i);
  input.version=1;input.state=TT_HOST_PAGE_READY;input.allocated=input.memory_type_set=input.map_stable=1;
  input.patterns=2;input.physical=physical[0];input.bytes=TT_HOST_PAGE_BYTES;
  bulk.allocated=bulk.ready=bulk.memory_type_set=true;bulk.state=TT_OUTPUT_BUFFER_READY;bulk.patterns=4;bulk.physical=physical[1];
  wide.allocated=wide.ready=wide.memory_type_set=true;wide.state=TT_OUTPUT_BUFFER_READY;wide.patterns=4;wide.physical=physical[2];
 }
 bool TokenViewLeaseIdentity(const PioneerQ38TokenViewLease& v)const{return &v==&views;}
 bool TokenHostLeaseIdentity(const PioneerQ38TokenHostLease& h)const{return &h==&hosts;}
 bool TokenViewCaptureScope(){return eligible;}bool TokenViewsScope(){return eligible;}
 bool TokenHostCaptureScope(){return eligible;}bool TokenHostReleaseScope(){return eligible;}
 int32_t* TokenViewHandle(unsigned i){return i<29?&device[i]:nullptr;}
 int32_t TokenHostView(unsigned i){return i<3?host[i]:-1;}
 uintptr_t TokenHostPointer(unsigned i){return i<3?pointer[i]:0;}
 uint64_t TokenHostPhysical(unsigned i){return i<3?physical[i]:0;}
 bool TokenHostAttempted(unsigned i){return i>=3 || attempted[i];}bool TokenHostQuarantined(unsigned i){return i>=3 || quarantine[i];}
 bool TokenViewDelete(int32_t){deviceCalls++;return true;}
 int32_t TokenHostReleaseChecked(unsigned i){
  assert(i<3 && hosts.PermitsRelease(i) && !attempted[i]);attempted[i]=true;calls.push_back(i);
  if(fail==calls.size())return -17;
  host[i]=-1;pointer[i]=0;
  if(lose==calls.size())eligible=false;
  if(mutate==calls.size())physical[0]+=4096;
  return 0;
 }
 bool Capture(){return views.Bind(*this) && hosts.Capture(*this,input,bulk,wide,views);}
 bool Match(){return hosts.Matches(*this,input,bulk,wide,views);}
 void DeviceClose(){assert(views.Close(*this) && views.DeviceClosureMatches(*this) && deviceCalls==29);}
};
int main(){
 unsigned cases=0;
 {
  IO io;PioneerQ38TokenHostLease foreign;
  assert(!foreign.Capture(io,io.input,io.bulk,io.wide,io.views) && foreign.Pristine() && !foreign.Close(io));
  assert(io.Capture() && io.hosts.Prepared() && io.Match() && !foreign.Matches(io,io.input,io.bulk,io.wide,io.views));
  assert(!io.hosts.Close(io) && io.calls.empty() && io.hosts.Prepared());
  io.DeviceClose();assert(io.hosts.Close(io) && io.hosts.Complete() && io.hosts.ReleasedCount()==3 && io.Match());
  assert((io.calls==std::vector<unsigned>{2,1,0}) && io.input.released && io.bulk.released && io.wide.released && !io.bulk.ready && !io.wide.ready);
  assert(!io.views.Matches(io) && io.views.DeviceClosureMatches(io)); // strict retained-host match is NOT broadened
  assert(!io.hosts.Close(io) && io.calls.size()==3 && !io.hosts.Capture(io,io.input,io.bulk,io.wide,io.views));cases++;
 }
 for(unsigned kind=0;kind<3;kind++)for(unsigned nth=1;nth<=3;nth++){
  IO io;assert(io.Capture());io.DeviceClose();
  if(kind==0)io.fail=nth;if(kind==1)io.lose=nth;if(kind==2)io.mutate=nth;
  assert(!io.hosts.Close(io) && io.hosts.Unsafe() && !io.hosts.Complete() && io.calls.size()==nth);
  const unsigned successes=nth-(kind==0);assert(io.hosts.ReleasedCount()==successes);
  for(unsigned i=0;i<3;i++)assert(io.host[i]==(i>=3-successes?-1:int32_t(301+i)));
  assert(io.input.released==unsigned(successes==3) && io.bulk.released==(successes>=2) && io.wide.released==(successes>=1));
  io.eligible=true;io.fail=io.lose=io.mutate=0;assert(!io.hosts.Close(io) && io.calls.size()==nth);cases++;
 }
 for(unsigned i=0;i<3;i++)for(unsigned kind=0;kind<8;kind++){
  IO io;assert(io.Capture());io.DeviceClose();
  if(kind==0)io.host[i]++;if(kind==1)io.host[i]=-1;if(kind==2)io.pointer[i]+=4;if(kind==3)io.pointer[i]=0;
  if(kind==4)io.physical[i]+=4096;if(kind==5)io.attempted[i]=true;if(kind==6)io.quarantine[i]=true;if(kind==7)io.device[28]=987;
  assert((kind==7 || !io.Match()) && !io.hosts.Close(io) && io.calls.empty());cases++;
 }
 for(unsigned which=0;which<30;which++){
  IO io;assert(io.Capture());io.DeviceClose();
  if(which==0)io.input.version=2;if(which==1)io.input.state=0;if(which==2)io.input.allocated=0;
  if(which==3)io.input.memory_type_set=0;if(which==4)io.input.map_stable=0;if(which==5)io.input.patterns=1;
  if(which==6)io.input.mismatches=1;if(which==7)io.input.bytes=4095;if(which==8)io.input.release_status=-1;
  if(which==9)io.input.allocation_status=-1;if(which==10)io.input.map_status=-1;if(which==11)io.input.cache_status=-1;
  if(which==12)io.input.released=1;if(which==13)io.input.physical+=4096;
  auto& out=which<22?io.bulk:static_cast<PioneerOutputBufferLease&>(io.wide);const unsigned field=which<22?which-14:which-22;
  if(which>=14){if(field==0)out.allocated=false;if(field==1)out.ready=false;if(field==2)out.memory_type_set=false;if(field==3)out.pending=true;
   if(field==4)out.quarantined=true;if(field==5)out.state=0;if(field==6)out.patterns=3;if(field==7)out.mismatches=1;}
  assert(!io.Match() && !io.hosts.Close(io) && io.calls.empty());cases++;
 }
 for(unsigned i=0;i<3;i++)for(unsigned kind=0;kind<5;kind++){
  IO io;assert(io.views.Bind(io));if(kind==0)io.pointer[i]=0;if(kind==1)io.pointer[i]=UINTPTR_MAX-3;
  if(kind==2)io.attempted[i]=true;if(kind==3)io.quarantine[i]=true;if(kind==4)io.pointer[i]=io.pointer[(i+1)%3];
  assert(!io.hosts.Capture(io,io.input,io.bulk,io.wide,io.views) && !io.hosts.Complete() && !io.hosts.Close(io) && io.calls.empty());cases++;
 }
 for(unsigned i=0;i<29;i++){
  IO io;assert(io.Capture());io.DeviceClose();io.device[i]=int32_t(400+i);
  assert(!io.hosts.Close(io) && io.calls.empty());cases++;
 }
 for(unsigned kind=0;kind<2;kind++)for(unsigned fault=0;fault<3;fault++){
  IO io;assert(io.Capture());io.DeviceClose();auto& out=kind?static_cast<PioneerOutputBufferLease&>(io.wide):io.bulk;
  if(fault==0)out.released=true;if(fault==1)out.physical+=4096;if(fault==2)out.patterns=5;
  assert(!io.Match() && !io.hosts.Close(io) && io.calls.empty());cases++;
 }
 for(unsigned kind=0;kind<3;kind++){
  IO io;assert(io.Capture());io.DeviceClose();auto h=io.input;auto b=io.bulk;auto w=io.wide;
  assert(!io.hosts.Matches(io,kind==0?h:io.input,kind==1?b:io.bulk,kind==2?w:io.wide,io.views));cases++;
 }
 {
  IO io;assert(io.Capture());io.DeviceClose();auto copy=io.input;
  assert(!io.hosts.Matches(io,copy,io.bulk,io.wide,io.views));io.eligible=false;
  assert(!io.hosts.Close(io) && io.calls.empty());cases++;
 }
 printf("PASS wired-host journal %u cases:SAME private prelaunch handles+virtual/physical identities+lease metadata/29-device-closure prerequisite/exact wide->bulk->input OS release/every failure+post-success lifetime-loss+metadata mutation/known released prefix recorded/NO later release,retry or invented full END;component DATA tests ONLY,not native/physical/Work/TPS qualification\n",cases);
}
