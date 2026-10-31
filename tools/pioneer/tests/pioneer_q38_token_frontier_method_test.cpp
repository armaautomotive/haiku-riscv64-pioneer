/* SPDX-License-Identifier: MIT */
// Actual extracted observation/Fail methods ONLY. Scope,source,boot seal
// and owner quarantine are explicit MOCK authority,not native lifetimes.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "actual_frontier_result.inc"
namespace PioneerQ38GlobalChainLayout {namespace R {enum {Status=0x75440};}}
namespace PioneerQ38TokenLayout {enum {Magic=0x31545351};}
namespace PioneerQ38TokenPrograms {enum {FrontierAddress=0x7570c};}
struct CounterOwner;
struct CounterIO {
 CounterOwner* owner=nullptr;unsigned touches=0,reads=0,failRead=0,failSource=0,sources=0;
 uint32_t marker=PioneerQ38TokenLayout::Magic,error=0,count=0;
 bool boot=true;unsigned failAt=0;
 void Touch();bool TokenMemoryReadChecked(unsigned,uint32_t,uint32_t&);
};
struct CounterOwner {
 bool frontierZeroed=true,holding=false,held=false,terminalObserved=false,unsafe=false,execution=true,scope=true;
 uint32_t frontierLimit=34,frontierPrompts=2,frontierLast=0,observedTerminal[64]={};
 unsigned retained=0;
 bool ExecutionStarted()const{return execution && !unsafe;}
 bool Scope(CounterIO&)const{return scope && !unsafe;}
 bool Sources(CounterIO& io,unsigned worker){
  assert(!worker);io.Touch();io.sources++;return Scope(io) && io.sources!=io.failSource;
 }
 bool RootBootSeal(CounterIO& io,uint32_t& word){io.Touch();word=63;return io.boot;}
 bool FailOwner(CounterIO& io){retained++;unsafe=true;io.failAt=io.touches;return false;}
#include "actual_frontier_methods.inc"
};
void CounterIO::Touch(){assert(owner && !owner->unsafe && !failAt);touches++;}
bool CounterIO::TokenMemoryReadChecked(unsigned worker,uint32_t address,uint32_t& output){
 assert(!worker);Touch();reads++;
 if(reads==failRead)return false;
 if(address==PioneerQ38GlobalChainLayout::R::Status+12)output=marker;
 else if(address==PioneerQ38GlobalChainLayout::R::Status+8)output=error;
 else {assert(address==PioneerQ38TokenPrograms::FrontierAddress);output=count;}
 return true;
} // no write,NoC command,reset,HOLD,free or lifecycle interface is supplied
struct Fixture {
 CounterOwner owner;CounterIO io{&owner};
 void Refuse(){
  const auto before=owner;const unsigned touches=io.touches;
  PioneerQ38TokenFrontierResult result;const auto pristine=result;
  assert(!owner.ObserveCommittedFrontier(io,result) && !memcmp(&result,&pristine,sizeof(result))
   && io.touches==touches && owner.retained==before.retained && owner.frontierLast==before.frontierLast);
 }
 void Fail(){
  const bool terminal=owner.terminalObserved,held=owner.held,holding=owner.holding;
  const uint32_t previous=owner.frontierLast;PioneerQ38TokenFrontierResult result;
  assert(!owner.ObserveCommittedFrontier(io,result) && result.started && result.unsafe && !result.ready
   && !result.committed && !result.limit && !result.prompts && owner.unsafe && owner.retained==1
   && io.failAt==io.touches && owner.frontierLast==previous && owner.terminalObserved==terminal
   && owner.held==held && owner.holding==holding);
  const unsigned touches=io.touches;PioneerQ38TokenFrontierResult again;
  assert(!owner.ObserveCommittedFrontier(io,again) && io.touches==touches && owner.retained==1);
 }
 void Read(unsigned count){
  io.count=count;const bool terminal=owner.terminalObserved;PioneerQ38TokenFrontierResult result;
  assert(owner.ObserveCommittedFrontier(io,result) && result.started && result.ready && !result.unsafe
   && result.committed==count && result.limit==owner.frontierLimit && result.prompts==owner.frontierPrompts
   && owner.frontierLast==count && owner.terminalObserved==terminal && !owner.held && !owner.holding && !owner.retained);
  for(auto value:result.reserved)assert(!value);
 }
};
int main(){
 unsigned positive=0,refused=0,failed=0;
 {
  Fixture f;f.owner.frontierLimit=8192;
  for(unsigned count=0;count<=8192;count++){f.Read(count);positive++;}
  f.Read(8192);positive++;
 }
 {Fixture f;f.Read(0);f.Read(17);f.Read(34);positive+=3;}
 {Fixture f;f.owner.terminalObserved=true;f.owner.observedTerminal[3]=34;f.Read(34);positive++;}
 for(unsigned field=0;field<16;field++){
  Fixture f;PioneerQ38TokenFrontierResult result;reinterpret_cast<uint32_t*>(&result)[field]^=1;
  const auto before=result;assert(!f.owner.ObserveCommittedFrontier(f.io,result) && !f.io.touches
   && !f.owner.retained && !memcmp(&result,&before,sizeof(result)));refused++;
 }
 for(unsigned condition=0;condition<10;condition++){
  Fixture f;
  switch(condition){
   case 0:f.owner.frontierZeroed=false;break;case 1:f.owner.frontierLimit=0;break;
   case 2:f.owner.frontierLimit=8193;break;case 3:f.owner.frontierPrompts=0;break;
   case 4:f.owner.frontierPrompts=35;break;case 5:f.owner.holding=true;break;
   case 6:f.owner.held=true;break;case 7:f.owner.execution=false;break;
   case 8:f.owner.scope=false;break;case 9:f.owner.unsafe=true;break;default:assert(false);
  }
  f.Refuse();refused++;
 }
 for(unsigned condition=0;condition<10;condition++){
  Fixture f;
  switch(condition){
   case 0:f.io.marker=0;break;case 1:f.io.marker^=1;break;case 2:f.io.error=0x51465431;break;
   case 3:f.io.count=35;break;case 4:f.io.count=UINT32_MAX;break;
   case 5:f.owner.frontierLast=17;f.io.count=16;break;
   case 6:f.owner.terminalObserved=true;f.owner.observedTerminal[3]=33;f.io.count=34;break;
   case 7:f.io.failSource=1;break;case 8:f.io.failSource=2;break;
   case 9:f.io.boot=false;break;default:assert(false);
  }
  f.Fail();failed++;
 }
 for(unsigned read=1;read<=3;read++){Fixture f;f.io.failRead=read;f.Fail();failed++;}
 printf("PASS actual extracted frontier observation+Fail/%u bounded+repeat+jump+terminal DATA positives/%u pristine+fresh-zero+phase+scope refusals/%u marker+error+range+regression+terminal+source+boot+read failures/retained-once/no following IO/no write,command,reset,HOLD,END or release interface;MOCK authority ONLY,NOT genuine owner/model/physical/clock/TPS proof\n",positive,refused,failed);
}
