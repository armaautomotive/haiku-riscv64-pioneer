/* SPDX-License-Identifier: MIT */
// Actual extracted production HOLD and clock methods. Everything else is
// deliberately MOCK:neither a terminal transcript nor native authority.
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <array>
#include <vector>
#include "actual_hold_records.inc"
namespace PioneerQ38TokenLayout {enum {TerminalLocal=0x75c00};}
struct MethodMock;
struct ClockIO {
 MethodMock* owner=nullptr;uint64_t now=100,pauseStep=1000;unsigned pauses=0,clockReads=0;
 uint64_t Q38CohortNow(){clockReads++;return now;}
 void Pause(){pauses++;now+=pauseStep;}
 void RuntimeResetWrite(uint32_t);
 bool TokenMemoryReadChecked(unsigned,uint32_t,uint32_t&);
};
struct MethodMock {
 struct Image {bool running=true;};struct Worker {Image image;};
 struct Cohort {Worker worker[120];bool rootRunning=true;} owned;
 Cohort* cohort=&owned;
 bool holding=false,held=false,unsafe=false,terminal=true,execution=true,scope=true;
 bool running[120];uint32_t reset[120],observedTerminal[64]={};
 unsigned inspections=0,pending=0,faultAt=0,readyAt=0,guardCalls=0,failures=0,scopeLostAt=0;
 bool lateReady=false,regressClock=false;std::vector<unsigned> holds;
 MethodMock(){for(unsigned w=0;w<120;w++){running[w]=true;reset[w]=0x46800;}for(unsigned i=0;i<64;i++)observedTerminal[i]=i+7;}
 bool TerminalObserved()const{return terminal && !unsafe;}
 bool ExecutionStarted()const{return execution && !unsafe;}
 bool Scope(ClockIO&)const{return scope && !unsafe;}
 bool Fail(ClockIO&,PioneerQ38TokenHoldResult& r){unsafe=true;r.unsafe=1;r.ready=0;failures++;return false;}
 bool InspectTerminal(ClockIO& io,PioneerQ38TokenTerminalResult& r,bool recheck){
  assert(recheck && !unsafe && holds.empty() && !readyAt);inspections++;
  r.started=1;r.phase=9;r.worker=37;r.word=19;r.firstPC=0x40010;r.secondPC=0x40014;
  if(faultAt==inspections){unsafe=true;r.unsafe=1;return false;}
  if(scopeLostAt==inspections)scope=false;
  if(inspections<=pending)return true;
  r.phase=11;r.ready=1;r.worker=0;r.word=63;r.firstPC=r.secondPC=0x60004;readyAt=inspections;
  if(lateReady)io.now+=30000000;
  if(regressClock)io.now--;
  return true;
 }
 bool HoldGuard(ClockIO&,unsigned w,PioneerQ38TokenHoldResult& r){
  assert(!unsafe && readyAt && w<120);guardCalls++;
  if(running[w])r.firstPC=r.secondPC=w?0x20060:0x60004;
  return true;
 }
 bool Sources(ClockIO&,unsigned w){assert(!unsafe && w==0 && holds.size()==120);return true;}
#include "actual_hold_methods.inc"
};
void ClockIO::RuntimeResetWrite(uint32_t value){
 assert(owner && !owner->unsafe && owner->readyAt && value==0x47800);
 const unsigned w=owner->holds.size();assert(w<120 && !owner->running[w] && owner->reset[w]==value);
 owner->holds.push_back(w);
}
bool ClockIO::TokenMemoryReadChecked(unsigned w,uint32_t address,uint32_t& value){
 assert(owner && !owner->unsafe && w==0 && owner->holds.size()==120 && address>=PioneerQ38TokenLayout::TerminalLocal);
 const unsigned at=(address-PioneerQ38TokenLayout::TerminalLocal)/4;assert(at<64);value=owner->observedTerminal[at];return true;
}
static void Clean(const MethodMock& m,const PioneerQ38TokenHoldResult& r){
 assert(!m.unsafe && m.held && !m.holding && r.ready && r.phase==4 && r.held==120 && m.holds.size()==120);
 for(unsigned w=0;w<120;w++)assert(m.holds[w]==w && !m.running[w] && m.reset[w]==0x47800 && !m.owned.worker[w].image.running);
 for(auto word:r.reserved)assert(!word);
}
int main(){
 unsigned positives=0,refusals=0;
 // This reproduces the OLD logical pending-as-fault branch using MOCK
 // observations. It is NOT a diagnosis of the physical failure's cause.
 {MethodMock m;ClockIO io;io.owner=&m;m.pending=1;PioneerQ38TokenHoldResult r;
  assert(!m.OldHoldStopped(io,r) && m.unsafe && r.phase==1 && !r.held && m.holds.empty() && m.inspections==1);refusals++;}
 for(unsigned pending:{0U,1U,2U,17U,65535U}){
  MethodMock m;ClockIO io;io.owner=&m;io.pauseStep=pending>100?0:1000;m.pending=pending;PioneerQ38TokenHoldResult r;
  assert(m.HoldStopped(io,r));Clean(m,r);assert(m.inspections==pending+1 && io.pauses==pending+120);positives++;
 }
 {MethodMock m;ClockIO io;io.owner=&m;io.pauseStep=0;m.pending=65536;PioneerQ38TokenHoldResult r;
  assert(!m.HoldStopped(io,r) && m.unsafe && r.phase==1 && !r.held && m.inspections==65536 && m.holds.empty());refusals++;}
 for(unsigned at:{1U,2U,18U}){
  MethodMock m;ClockIO io;io.owner=&m;m.pending=32;m.faultAt=at;PioneerQ38TokenHoldResult r;
  assert(!m.HoldStopped(io,r) && m.unsafe && m.inspections==at && m.holds.empty() && !r.held
   && r.worker==37 && r.word==19 && r.firstPC==0x40010 && r.secondPC==0x40014 && io.pauses==at-1);refusals++;
 }
 for(unsigned kind=0;kind<5;kind++){
  MethodMock m;ClockIO io;io.owner=&m;PioneerQ38TokenHoldResult r;
  if(kind==0){m.pending=1;io.pauseStep=30000000;}
  if(kind==1){m.pending=1;io.pauseStep=UINT64_MAX;}
  if(kind==2)io.now=UINT64_MAX-29999999;
  if(kind==3)m.lateReady=true;
  if(kind==4)m.regressClock=true;
  assert(!m.HoldStopped(io,r) && m.unsafe && m.holds.empty() && !r.held && r.phase==1);refusals++;
 }
 for(unsigned pending:{0U,1U}){
  MethodMock m;ClockIO io;io.owner=&m;m.pending=pending;m.scopeLostAt=1;PioneerQ38TokenHoldResult r;
  assert(!m.HoldStopped(io,r) && m.unsafe && m.inspections==1 && m.holds.empty() && !r.held && !io.pauses);refusals++;
 }
 for(unsigned word=0;word<16;word++){
  MethodMock m;ClockIO io;io.owner=&m;PioneerQ38TokenHoldResult r;reinterpret_cast<uint32_t*>(&r)[word]^=1;
  assert(!m.HoldStopped(io,r) && !m.unsafe && !io.clockReads && !m.inspections && m.holds.empty());refusals++;
 }
 for(unsigned phase=0;phase<5;phase++){
  MethodMock m;ClockIO io;io.owner=&m;PioneerQ38TokenHoldResult r;
  if(phase==0)m.holding=true;if(phase==1)m.held=true;if(phase==2)m.terminal=false;if(phase==3)m.execution=false;if(phase==4)m.scope=false;
  assert(!m.HoldStopped(io,r) && !m.unsafe && !io.clockReads && !m.inspections && m.holds.empty());refusals++;
 }
 std::printf("PASS actual extracted HOLD+UNCHANGED clock guard/%u MOCK positives/%u refusals/old pending-as-fault reproduced/new bounded pending1..65535 then exact120 HOLD order;false inspection,timeout,clock regression/exhaustion cause NO HOLD or later observation;failed inner worker/word/PC DATA retained;NO native,model,physical,Work or TPS proof\n",positives,refusals);
}
