/* SPDX-License-Identifier: MIT */
// HOST transport endpoints, not a native owner or physical delivery claim.
#define PIONEER_Q38_POSTED_CANCEL_TEST 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38PostedCancel.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
struct PostedCancelEndpoint {
 using L=PioneerQ38PostedCancel;
 uint32_t tlb[2][3]={},cell=0;
 unsigned raw=0,failAfter=0,sourceChecks=0,sourceFailure=0,postWrites=0,modeWrites=0,reads=0,delay=2;
 uint64_t clock=100;
 bool live=true,pending=false,neverDeliver=false,wrongValue=false,dropPostMode=false,dropRestore=false;
 unsigned clockFault=0,clockReads=0;
 std::vector<unsigned> events;
 std::array<uint32_t,4> nonposted={{17,18,19,20}},posted={{31,32,33,34}};
 PostedCancelEndpoint(){for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)tlb[s][w]=L::Expected(s,w,false);}
 bool PostedCancelScope(const L&){return live;}
 void Event(unsigned kind){assert(live);events.push_back(kind);raw++;if(raw==failAfter)live=false;}
 bool PostedCancelTlbRead(unsigned s,unsigned w,uint32_t& out){
  assert(s<2 && w<3);out=tlb[s][w];Event(1);return true;
 }
 bool PostedCancelSourceGuard(){sourceChecks++;Event(2);return sourceChecks!=sourceFailure;}
 bool PostedCancelControlWrite(bool post){
  assert(tlb[0][2]==L::Expected(0,2,!post));modeWrites++;
  if(!(post?dropPostMode:dropRestore))tlb[0][2]=L::Expected(0,2,post);
  Event(post?3:4);return true;
 }
 bool PostedCancelWordWrite(){
  assert(!postWrites && !pending && !cell && tlb[0][2]==L::Expected(0,2,true));
  postWrites++;pending=true;Event(5);return true;
 }
 bool PostedCancelWordRead(uint32_t& out){
  if(pending && !neverDeliver && ++reads>delay){
   cell=wrongValue?1U:unsigned(PioneerQ38TokenLayout::CancelMagic);pending=false;
   for(unsigned i=0;i<3;i++)posted[i]++; // posted STARTED/flit/RECEIVED,NO ACK
  }
  out=cell;Event(6);return true;
 }
 uint64_t PostedCancelNow(){
  clockReads++;
  if(clockFault==1 && clockReads==1)return UINT64_MAX;
  if(clockFault==2 && clockReads==2)return 99;
  if(clockFault==3 && clockReads==2)return 30000100;
  return clock;
 }
 void PostedCancelPause(){assert(live);clock+=1000;}
 bool Normal()const{for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)if(tlb[s][w]!=L::Expected(s,w,false))return false;return true;}
};
int main(){
 using L=PioneerQ38PostedCancel;unsigned cases=0;
 PostedCancelEndpoint okay;L request;const auto originalIncoming=okay.nonposted;
 assert(request.Request(okay) && request.Delivered() && !request.Active() && !request.Failed()
  && !request.Changed() && okay.Normal() && okay.postWrites==1 && okay.modeWrites==2
  && okay.reads==3 && okay.cell==PioneerQ38TokenLayout::CancelMagic && okay.nonposted==originalIncoming);
 assert(okay.posted[0]==32 && okay.posted[1]==33 && okay.posted[2]==34 && okay.posted[3]==34);
 const unsigned complete=okay.raw;assert(!request.Request(okay) && okay.raw==complete);cases++;
 // Scope loss AFTER each actual raw endpoint event. Never restore/retry
 // or perform another MMIO operation after the first observed uncertainty.
 for(unsigned event=1;event<=complete;event++){
  PostedCancelEndpoint io;L candidate;io.failAfter=event;
  assert(!candidate.Request(io) && candidate.Failed() && candidate.Active() && !candidate.Delivered()
   && io.raw==event && io.postWrites<=1 && io.modeWrites<=2);
  const auto saved=io.events;assert(!candidate.Request(io) && io.events==saved);cases++;
 }
 for(unsigned slot=0;slot<2;slot++)for(unsigned word=0;word<3;word++)for(unsigned bit=0;bit<32;bit++){
  PostedCancelEndpoint io;L candidate;io.tlb[slot][word]^=1U<<bit;
  assert(!candidate.Request(io) && candidate.Failed() && !io.modeWrites && !io.postWrites);cases++;
 }
 for(unsigned which=0;which<12;which++){
  PostedCancelEndpoint io;L candidate;
  if(which<4)io.sourceFailure=which+1;
  else if(which==4)io.dropPostMode=true;
  else if(which==5)io.dropRestore=true;
  else if(which==6)io.neverDeliver=true;
  else if(which==7)io.wrongValue=true;
  else if(which<11)io.clockFault=which-7;
  else io.cell=PioneerQ38TokenLayout::CancelMagic;
  assert(!candidate.Request(io) && candidate.Failed() && !candidate.Delivered() && io.postWrites<=1);
  const unsigned stopped=io.raw;assert(!candidate.Request(io) && io.raw==stopped);cases++;
 }
 printf("PASS posted cancel controller/%u cases/one fixed write/delayed real endpoint delivery readback BEFORE normal-mode return/strict incoming counters unchanged/all raw-event scope losses+192 route mutations+source/mode/readback/clock/poll faults fail closed/no retry or failure cleanup;HOST endpoints only,NOT native/physical/Work/TPS qualification\n",cases);
}
