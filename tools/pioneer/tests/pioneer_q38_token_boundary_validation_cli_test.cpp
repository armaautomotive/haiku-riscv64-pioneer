/* SPDX-License-Identifier: MIT */
// Actual argument and delay helpers;no source/model/device access.
#include "../PioneerQ38TokenBoundaryValidationCLI.h"
#include "../PioneerQ38TokenBoundaryDelay.h"
#include "../PioneerQ38TokenBoundaryAbort.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <initializer_list>
struct Clock {
 int64_t before=100000,after=600000,duration=0;unsigned reads=0,sleeps=0;bool slept=true;
 int64_t Now(){return reads++?after:before;}
 bool Sleep(int64_t d){duration=d;sleeps++;return slept;}
};
struct AbortedSession {
 const char* failure=nullptr;unsigned aborts=0;
 void Abort(){aborts++;if(!failure)failure="aborted";}
};
int main(){
 PioneerQ38TokenBoundaryValidationOptions o;o.output="sentinel";o.delayMs=17;
 unsigned accepted=0,refused=0;
 for(const char* ms:{"0","1","500","5000"}){
  const char* a[]={"client","--capture-layer-boundary",ms,"/private/tmp/test.qhb"};
  assert(PioneerQ38TokenBoundaryValidationArguments(4,a,o) && !o.preflight && o.output==a[3]);accepted++;
 }
 for(const char* ms:{"","-1","+1","00","01"," 1","1 ","1ms","0x1","5001","999999999999999999999999999999"}){
  const auto before=o;const char* a[]={"client","--capture-layer-boundary",ms,"/private/tmp/test.qhb"};
  assert(!PioneerQ38TokenBoundaryValidationArguments(4,a,o) && o.output==before.output && o.delayMs==before.delayMs && o.preflight==before.preflight);refused++;
 }
 for(const char* path:{"","/","relative","/private/tmp/"}){
  const char* a[]={"client","--capture-layer-boundary","500",path};assert(!PioneerQ38TokenBoundaryValidationArguments(4,a,o));refused++;
 }
 const char* args[]={"client","--capture-layer-boundary","500","/private/tmp/test.qhb","extra"};
 for(int count:{0,1,2,3,5}){assert(!PioneerQ38TokenBoundaryValidationArguments(count,args,o));refused++;}
 for(const char* mode:{"--capture-first-layer","--validate-three-positions","--validate-cancel-boundary","--host-timing","--work"}){
  const char* a[]={"client",mode,"500","/private/tmp/test.qhb"};assert(!PioneerQ38TokenBoundaryValidationArguments(4,a,o));refused++;
 }
 const char* pre[]={"client","--preflight-source-only"};assert(PioneerQ38TokenBoundaryValidationArguments(2,pre,o) && o.preflight && !o.output && !o.delayMs);accepted++;
 for(unsigned ms:{0U,1U,500U,5000U}){
  Clock c;c.after=c.before+int64_t(ms)*1000+17;int64_t end=-91;
  assert(PioneerQ38TokenBoundaryDelay(c,ms,end) && end==c.after && c.reads==2 && c.sleeps==unsigned(ms!=0) && c.duration==int64_t(ms)*1000);accepted++;
 }
 for(unsigned fault=0;fault<6;fault++){
  Clock c;int64_t end=-91;unsigned ms=500;
  if(fault==0)ms=5001;
  if(fault==1)c.before=-1;
  if(fault==2)c.before=INT64_MAX-100;
  if(fault==3)c.slept=false;
  if(fault==4)c.after=c.before-1;
  if(fault==5)c.after=c.before+499999;
  assert(!PioneerQ38TokenBoundaryDelay(c,ms,end) && end==-91 && c.sleeps<=1 && c.reads<=2);refused++;
 }
 unsigned reasons=0;
 const char* failures[]={nullptr,"observe","finish"};
 const char* observations[]={nullptr,"deadline_or_clock","snooze","clock"};
 for(const char* failure:failures)for(const char* observation:observations){
  AbortedSession session;session.failure=failure;
  const char* expected=failure?failure:observation?observation:"unknown";
  const char* reported=PioneerQ38TokenBoundaryAbortReason(session,observation);
  assert(!strcmp(reported,expected) && session.aborts==1 && !strcmp(session.failure,failure?failure:"aborted"));reasons++;
 }
 printf("PASS observed-boundary CLI+delay/%u positives/%u decimal+range+old-mode+path+clock+sleep refusals/%u first-cause-before-Abort cases/no output mutation/retry/device access/NO exact-layer selection;pure helpers only,not physical/numerical/Work/TPS proof\n",accepted,refused,reasons);
}
