/* SPDX-License-Identifier: MIT */
// Two ACTUAL extracted observer/Fail bodies. Authority is explicitly MOCK.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include "actual_frontier_result.inc"
#define B_DEVICE_OP_CODES_END 0x7000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenFrontierPacket.h"
#include "actual_frontier_failure_formatter.inc"
namespace PioneerQ38GlobalChainLayout {namespace R {enum {Status=0x75440};}}
namespace PioneerQ38TokenLayout {enum {Magic=0x31545351};}
namespace PioneerQ38TokenPrograms {enum {FrontierAddress=0x7570c};}
struct State;
struct IO {
 State* owner=nullptr;
 explicit IO(State* state):owner(state){}
 std::vector<uint32_t> calls;
 unsigned reads=0,sources=0,failRead=0,failSource=0;
 uint32_t marker=PioneerQ38TokenLayout::Magic,error=0,count=0;
 bool boot=true;
 void Touch(uint32_t);
 bool TokenMemoryReadChecked(unsigned,uint32_t,uint32_t&);
};
struct State {
 bool frontierZeroed=true,holding=false,held=false,terminalObserved=false,unsafe=false,execution=true,scope=true;
 uint32_t frontierLimit=34,frontierPrompts=2,frontierLast=0,observedTerminal[64]={};
 unsigned retained=0;
 bool ExecutionStarted()const{return execution && !unsafe;}
 bool Scope(IO&)const{return scope && !unsafe;}
 bool Sources(IO& io,unsigned worker){
  assert(!worker);io.Touch(1);io.sources++;return Scope(io) && io.sources!=io.failSource;
 }
 bool RootBootSeal(IO& io,uint32_t& word){io.Touch(2);word=63;return io.boot;}
 bool FailOwner(IO&){retained++;unsafe=true;return false;}
};
struct Original:State {
#include "actual_frontier_methods.inc"
};
#define PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE 1
struct Detailed:State {
#include "actual_frontier_methods.inc"
};
#undef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE
void IO::Touch(uint32_t value){assert(owner && !owner->unsafe);calls.push_back(value);}
bool IO::TokenMemoryReadChecked(unsigned worker,uint32_t address,uint32_t& output){
 assert(!worker);Touch(address);reads++;
 if(reads==failRead){output=0xd15ca4d;return false;} // deliberately UNTRUSTED partial output
 if(address==PioneerQ38GlobalChainLayout::R::Status+12)output=marker;
 else if(address==PioneerQ38GlobalChainLayout::R::Status+8)output=error;
 else {assert(address==PioneerQ38TokenPrograms::FrontierAddress);output=count;}
 return true;
}
struct Pair {
 Original old;Detailed detail;IO a{&old},b{&detail};
 void Configure(unsigned n){
  switch(n){
   case 0:b.failSource=1;break;
   case 1:b.failRead=1;break;
   case 2:b.marker=0;break;
   case 3:b.marker^=1;break;
   case 4:b.failRead=2;break;
   case 5:b.error=0x24;break;
   case 6:b.error=UINT32_MAX;break;
   case 7:b.failRead=3;break;
   case 8:b.failSource=2;break;
   case 9:b.boot=false;break;
   case 10:detail.frontierLast=17;b.count=16;break;
   case 11:b.count=35;break;
   case 12:b.count=UINT32_MAX;break;
   case 13:detail.terminalObserved=true;detail.observedTerminal[3]=33;b.count=34;break;
   // Check first-failure precedence:NEVER collect extra evidence after one fails.
   case 14:b.failSource=1;b.marker=0;b.error=7;b.count=100;break;
   case 15:b.failRead=1;b.marker=0;b.error=7;break;
   case 16:b.marker=0;b.error=7;break;
   case 17:b.failRead=2;b.error=7;break;
   case 18:b.error=7;b.failRead=3;break;
   case 19:b.failRead=3;b.failSource=2;break;
   case 20:b.failSource=2;b.boot=false;break;
   case 21:b.boot=false;b.count=UINT32_MAX;break;
   default:assert(false);
  }
  static_cast<State&>(old)=static_cast<State&>(detail);
  a=b;a.owner=&old;
 }
 void SameState(){
  assert(old.frontierLast==detail.frontierLast && old.unsafe==detail.unsafe && old.retained==detail.retained
   && old.terminalObserved==detail.terminalObserved && old.held==detail.held && old.holding==detail.holding);
  assert(a.calls==b.calls && a.reads==b.reads && a.sources==b.sources);
 }
 void Failed(unsigned step,unsigned known){
  PioneerQ38TokenFrontierResult before,after;
  const unsigned previous=detail.frontierLast;
  assert(!old.ObserveCommittedFrontier(a,before) && !detail.ObserveCommittedFrontier(b,after));
  assert(!memcmp(&before,&after,10*sizeof(uint32_t))); // all pre-existing result fields EXACT
  for(auto value:before.reserved)assert(!value);
  const uint32_t expected[]={0x31444651,step,(known&1)?b.marker:0,(known&2)?b.error:0,(known&4)?b.count:0,known};
  assert(!memcmp(after.reserved,expected,sizeof(expected)));
  assert(after.started && after.unsafe && !after.ready && !after.committed && !after.limit && !after.prompts
   && detail.retained==1 && detail.frontierLast==previous);
  SameState();
  namespace P=PioneerQ38TokenFrontierControl;
  P::Packet packet;auto& reply=packet.reply;
  reply.version=P::Version;reply.operation=P::Observe;reply.status=-1;
  reply.started=1;reply.unsafe=1;memcpy(reply.detail,&after,sizeof(after));
  assert(!P::ReplyShape(packet)); // NEVER relax v1 successful-packet grammar for failure DATA
  const auto immutable=packet;FILE* log=std::tmpfile();assert(log);
  assert(PioneerQ38WriteFrontierFailedReply(log,TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE,&packet,sizeof(packet),-1,-1));
  assert(!memcmp(&packet,&immutable,sizeof(packet)) && !std::fflush(log) && !std::fseek(log,0,SEEK_SET));
  char text[4096]={};const size_t bytes=std::fread(text,1,sizeof(text)-1,log);assert(bytes && !std::ferror(log));
  assert(!std::fclose(log) && std::strstr(text,"consistent_shape0;"));
  uint32_t words[32];memcpy(words,&packet,sizeof(packet));
  for(unsigned row=0;row<4;row++){
   char expected[192];int used=std::snprintf(expected,sizeof(expected),"Q38_FRONTIER_FAILED_PACKET word%02u",row*8);
   assert(used>0);
   for(unsigned col=0;col<8;col++){
    int added=std::snprintf(expected+used,sizeof(expected)-unsigned(used)," %08x",unsigned(words[row*8+col]));
    assert(added==9);used+=added;
   }
   assert(std::strstr(text,expected));
  }
  const auto calls=b.calls;PioneerQ38TokenFrontierResult again,pristine;
  assert(!detail.ObserveCommittedFrontier(b,again) && !memcmp(&again,&pristine,sizeof(again))
   && b.calls==calls && detail.retained==1);
 }
 void Read(unsigned count){
  a.count=b.count=count;PioneerQ38TokenFrontierResult x,y;
  assert(old.ObserveCommittedFrontier(a,x) && detail.ObserveCommittedFrontier(b,y));
  assert(!memcmp(&x,&y,sizeof(x)) && y.ready && y.committed==count && !y.unsafe);
  for(auto value:y.reserved)assert(!value);
  SameState();
 }
};
int main(){
 unsigned successes=0,failures=0,refusals=0;
 {Pair p;p.old.frontierLimit=p.detail.frontierLimit=8192;
  for(unsigned count=0;count<=8192;count++){p.Read(count);successes++;}
  p.Read(8192);successes++;
 }
 {Pair p;p.Read(0);p.Read(17);p.Read(34);successes+=3;}
 {Pair p;p.old.terminalObserved=p.detail.terminalObserved=true;
  p.old.observedTerminal[3]=p.detail.observedTerminal[3]=34;p.Read(34);successes++;
 }
 const unsigned stages[]={1,2,3,3,4,5,5,6,7,8,9,10,10,11,1,2,3,4,5,6,7,8};
 const unsigned masks[]= {0,0,1,1,1,3,3,3,7,7,7, 7, 7, 7,0,0,1,1,3,3,7,7};
 for(unsigned n=0;n<sizeof(stages)/sizeof(stages[0]);n++){Pair p;p.Configure(n);p.Failed(stages[n],masks[n]);failures++;}
 for(unsigned word=0;word<16;word++){
  Pair p;PioneerQ38TokenFrontierResult result;reinterpret_cast<uint32_t*>(&result)[word]^=1;
  const auto copy=result;
  assert(!p.detail.ObserveCommittedFrontier(p.b,result) && !memcmp(&result,&copy,sizeof(result))
   && p.b.calls.empty() && !p.detail.retained);refusals++;
 }
 for(unsigned n=0;n<10;n++){
  Pair p;
  switch(n){
   case 0:p.detail.frontierZeroed=false;break;case 1:p.detail.frontierLimit=0;break;
   case 2:p.detail.frontierLimit=8193;break;case 3:p.detail.frontierPrompts=0;break;
   case 4:p.detail.frontierPrompts=35;break;case 5:p.detail.holding=true;break;
   case 6:p.detail.held=true;break;case 7:p.detail.execution=false;break;
   case 8:p.detail.scope=false;break;case 9:p.detail.unsafe=true;break;default:assert(false);
  }
  PioneerQ38TokenFrontierResult result,pristine;
  assert(!p.detail.ObserveCommittedFrontier(p.b,result) && !memcmp(&result,&pristine,sizeof(result))
   && p.b.calls.empty() && !p.detail.retained);refusals++;
 }
 printf("PASS ACTUAL original/detail observers:%u byte-exact successful observations/%u first-failure detail cases+actual formatter ALL32 words/%u pre-IO refusals;ALL ordered IO traces+original decisions identical/no extra reads/no IO after quarantine/failed-read outputs masked/v1 failure grammar NOT relaxed;MOCK authority,NOT native lifecycle,hardware or TPS proof\n",successes,failures,refusals);
}
