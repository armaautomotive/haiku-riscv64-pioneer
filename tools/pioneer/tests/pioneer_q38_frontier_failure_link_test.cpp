/* SPDX-License-Identifier: MIT */
// Reuse the UNCHANGED original/detail fixtures. Authority/IO remain MOCK.
#include "unchanged_original_detail_fixture.inc"
namespace PioneerQ38TokenPrograms {enum {FaultSavedLinkOffset=32,FaultMarker=0x51465431};}
#define PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE 1
#define PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_NATIVE 1
struct Linked:State {
#include "actual_frontier_methods.inc"
};
#undef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_NATIVE
#undef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE
struct LinkIO:IO {
 uint32_t link=0x4badc0de,capturedLink=0,lateLink=0;
 unsigned linkReads=0;bool publishLate=false;
 explicit LinkIO(State* owner):IO(owner){}
 bool TokenMemoryReadChecked(unsigned worker,uint32_t address,uint32_t& output){
  if(address==0x75460){
   assert(!worker);Touch(address);reads++;linkReads++;
   if(reads==failRead){output=0xd15ca4d;return false;}
   output=capturedLink=link;return true;
  }
  if(address==0x75448 && publishLate){link=lateLink;error=0x51465431;}
  return IO::TokenMemoryReadChecked(worker,address,output);
 }
};
static void Raw(const PioneerQ38TokenFrontierResult& result){
 namespace P=PioneerQ38TokenFrontierControl;
 P::Packet packet;packet.reply.version=P::Version;packet.reply.operation=P::Observe;
 packet.reply.status=-1;packet.reply.started=packet.reply.unsafe=1;
 memcpy(packet.reply.detail,&result,sizeof(result));assert(!P::ReplyShape(packet));
 const auto frozen=packet;FILE* log=std::tmpfile();assert(log);
 assert(PioneerQ38WriteFrontierFailedReply(log,TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE,&packet,sizeof(packet),-1,-1));
 assert(!memcmp(&packet,&frozen,sizeof(packet)) && !std::fflush(log) && !std::fseek(log,0,SEEK_SET));
 char text[4096]={};assert(std::fread(text,1,sizeof(text)-1,log)>0 && !std::ferror(log) && !std::fclose(log));
 assert(std::strstr(text,"consistent_shape0;"));uint32_t words[32];memcpy(words,&packet,sizeof(packet));
 for(unsigned row=0;row<4;row++){
  char expected[192];int used=std::snprintf(expected,sizeof(expected),"Q38_FRONTIER_FAILED_PACKET word%02u",row*8);
  assert(used>0);
  for(unsigned col=0;col<8;col++){int added=std::snprintf(expected+used,sizeof(expected)-unsigned(used)," %08x",unsigned(words[row*8+col]));assert(added==9);used+=added;}
  assert(std::strstr(text,expected));
 }
}
struct LinkPair {
 Original old;Linked next;IO a{&old};LinkIO b{&next};
 void Bind(){static_cast<State&>(old)=static_cast<State&>(next);}
 void Same(){
  assert(old.frontierLast==next.frontierLast && old.unsafe==next.unsafe && old.retained==next.retained
   && old.terminalObserved==next.terminalObserved && old.held==next.held && old.holding==next.holding);
  auto calls=b.calls;unsigned links=0;
  for(auto i=calls.begin();i!=calls.end();)if(*i==0x75460){links++;i=calls.erase(i);}else ++i;
  assert(links==b.linkReads && links<=1 && calls==a.calls && b.reads==a.reads+links && a.sources==b.sources);
  if(links){assert(b.calls.size()>=3 && b.calls[0]==1 && b.calls[1]==0x7544c && b.calls[2]==0x75460);}
 }
 void Failed(unsigned check,unsigned known){
  Bind();PioneerQ38TokenFrontierResult before,after;const unsigned previous=next.frontierLast;
  assert(!old.ObserveCommittedFrontier(a,before) && !next.ObserveCommittedFrontier(b,after));
  assert(!memcmp(&before,&after,10*sizeof(uint32_t)));
  const uint32_t expected[]={0x314c4651,check,(known&1)?b.marker:0,(known&2)?b.error:0,(known&4)?b.capturedLink:0,known};
  assert(!memcmp(after.reserved,expected,sizeof(expected)) && next.frontierLast==previous
   && after.unsafe && !after.ready && !after.committed && !after.limit && !after.prompts && next.retained==1);
  Same();Raw(after);const auto calls=b.calls;
  PioneerQ38TokenFrontierResult again,pristine;
  assert(!next.ObserveCommittedFrontier(b,again) && !memcmp(&again,&pristine,sizeof(again)) && b.calls==calls && next.retained==1);
 }
 void Read(unsigned count){
  a.count=b.count=count;PioneerQ38TokenFrontierResult x,y;
  assert(old.ObserveCommittedFrontier(a,x) && next.ObserveCommittedFrontier(b,y));
  assert(!memcmp(&x,&y,sizeof(x)) && y.ready && y.committed==count && !y.unsafe);
  for(auto value:y.reserved)assert(!value);
  // Same() compares one observation, not the prior cumulative trace.
  Same();a.calls.clear();b.calls.clear();a.reads=b.reads=a.sources=b.sources=b.linkReads=0;
 }
};
int main(){
 unsigned positives=0,failures=0,refusals=0;
 {LinkPair p;p.old.frontierLimit=p.next.frontierLimit=8192;
  for(unsigned count=0;count<=8192;count++){p.b.link=0x91e10da5U*count;p.Read(count);positives++;}
  p.Read(8192);positives++;
 }
 {LinkPair p;p.Read(0);p.Read(17);p.Read(34);positives+=3;}
 {LinkPair p;p.old.terminalObserved=p.next.terminalObserved=true;p.old.observedTerminal[3]=p.next.observedTerminal[3]=34;p.Read(34);positives++;}
 for(unsigned n=0;n<18;n++){
  LinkPair p;unsigned check=0,known=0;
  switch(n){
   case 0:p.a.failSource=p.b.failSource=1;check=1;break;
   case 1:p.a.failRead=p.b.failRead=1;check=2;break;
   case 2:p.a.marker=p.b.marker=0;check=3;known=1;break;
   case 3:p.a.failRead=2;p.b.failRead=3;check=4;known=5;break;
   case 4:p.a.error=p.b.error=0x51465431;check=5;known=7;break;
   case 5:p.a.error=p.b.error=UINT32_MAX;check=5;known=7;break;
   case 6:p.a.failRead=3;p.b.failRead=4;check=6;known=7;break;
   case 7:p.a.failSource=p.b.failSource=2;check=7;known=7;break;
   case 8:p.a.boot=p.b.boot=false;check=8;known=7;break;
   case 9:p.next.frontierLast=17;p.a.count=p.b.count=16;check=9;known=7;break;
   case 10:p.a.count=p.b.count=35;check=10;known=7;break;
   case 11:p.next.terminalObserved=true;p.next.observedTerminal[3]=33;p.a.count=p.b.count=34;check=11;known=7;break;
   case 12:p.a.failSource=p.b.failSource=1;p.a.marker=p.b.marker=0;p.a.error=p.b.error=7;check=1;break;
   case 13:p.a.marker=p.b.marker=0;p.a.failRead=2;p.b.failRead=2;check=3;known=1;break;
   case 14:p.a.failRead=2;p.b.failRead=3;p.a.error=p.b.error=7;check=4;known=5;break;
   case 15:p.a.error=p.b.error=7;p.a.failRead=3;p.b.failRead=4;check=5;known=7;break;
   case 16:p.a.failSource=p.b.failSource=2;p.a.boot=p.b.boot=false;check=7;known=7;break;
   case 17:p.a.boot=p.b.boot=false;p.a.count=p.b.count=UINT32_MAX;check=8;known=7;break;
   default:assert(false);
  }
  p.Failed(check,known);failures++;
 }
 for(unsigned bit=0;bit<32;bit++){
  LinkPair p;p.b.link=1U<<bit;p.a.error=p.b.error=0x51465431;p.Failed(5,7);failures++;
 }
 {LinkPair p;p.b.link=0;p.b.publishLate=true;p.b.lateLink=0x4abcd;p.a.error=0x51465431;
  p.Failed(5,7);assert(p.b.link==0x4abcd && p.b.capturedLink==0 && p.b.linkReads==1);failures++;
 } // publication race retains the earlier ZERO snapshot;never invent/resample a causal link
 for(bool laterFault:{false,true}){
  LinkPair p;p.b.failRead=2;if(laterFault)p.b.error=0x51465431;
  PioneerQ38TokenFrontierResult r;assert(!p.next.ObserveCommittedFrontier(p.b,r));
  const uint32_t expected[]={0x314c4651,12,PioneerQ38TokenLayout::Magic,0,0,1};
  assert(!memcmp(r.reserved,expected,sizeof(expected)) && p.next.retained==1 && p.b.reads==2
   && p.b.calls==std::vector<uint32_t>({1,0x7544c,0x75460}));Raw(r);failures++;
 }
 for(unsigned word=0;word<16;word++){
  LinkPair p;PioneerQ38TokenFrontierResult r;reinterpret_cast<uint32_t*>(&r)[word]^=1;const auto frozen=r;
  assert(!p.next.ObserveCommittedFrontier(p.b,r) && !memcmp(&r,&frozen,sizeof(r)) && p.b.calls.empty() && !p.next.retained);refusals++;
 }
 for(unsigned n=0;n<10;n++){
  LinkPair p;switch(n){
   case 0:p.next.frontierZeroed=false;break;case 1:p.next.frontierLimit=0;break;case 2:p.next.frontierLimit=8193;break;
   case 3:p.next.frontierPrompts=0;break;case 4:p.next.frontierPrompts=35;break;case 5:p.next.holding=true;break;
   case 6:p.next.held=true;break;case 7:p.next.execution=false;break;case 8:p.next.scope=false;break;case 9:p.next.unsafe=true;break;
  }
  PioneerQ38TokenFrontierResult r,pristine;
  assert(!p.next.ObserveCommittedFrontier(p.b,r) && !memcmp(&r,&pristine,sizeof(r)) && p.b.calls.empty() && !p.next.retained);refusals++;
 }
 printf("PASS ACTUAL original/QFL1 observers:%u byte-exact successes with ONE fixed pre-error LINK read/%u first-failure+ALL32 raw packet cases/%u pre-IO refusals;failed-read output masked,late-publication zero snapshot preserved,no IO after failure,no v1 grammar relaxation;MOCK authority,NOT native/physical cause or TPS\n",positives,failures,refusals);
}
