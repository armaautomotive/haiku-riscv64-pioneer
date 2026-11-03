/* SPDX-License-Identifier: MIT */
#include "../PioneerDramCohortHostBracket.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
using B=PioneerDramCohortHostBracket;
using R=PioneerDramCohortHostTiming;
static B Good(uint32_t nonce){
 B b;assert(b.Arm(nonce,100));
 assert(b.Observe(B::Ready,0,110,115));
 assert(b.Observe(B::Start,0,120,125));
 assert(b.Observe(B::Done,0,130,135));
 assert(b.Observe(B::Payload,0,140,145));
 assert(b.Observe(B::Ready,nonce,150,155));
 assert(b.Observe(B::Start,nonce,160,165));
 assert(b.Observe(B::Done,0,170,175));
 assert(b.Observe(B::Payload,nonce,180,185));
 assert(b.Observe(B::Ready,nonce,200,205));
 assert(b.Observe(B::Start,nonce,210,215));
 assert(b.Observe(B::Done,nonce,220,225));
 assert(b.Observe(B::Payload,~nonce,230,235));
 return b;
}
int main(){
 unsigned valid=0,unavailable=0,refused=0;
 for(uint32_t nonce:{1U,0x32435a67U,0x80000000U,0xffffffffU}){
  auto b=Good(nonce);R r;b.Finish(9999,7*8388608,r);
  assert(r.state==1 && r.startLowerUs==110 && r.startUpperUs==166
   && r.endLowerUs==170 && r.endUpperUs==236 && r.elapsedMinUs==4 && r.elapsedMaxUs==126
   && r.cycles==9999 && r.bytes==7*8388608);valid++;
  // Observing the completed value repeatedly must never tighten the upper
  // bracket using a later observation,or change the measured cycle count.
  assert(b.Observe(B::Payload,~nonce,250,260));R again;b.Finish(9999,7*8388608,again);
  assert(again.endUpperUs==r.endUpperUs && again.elapsedMinUs==4);valid++;
  for(unsigned missing=0;missing<5;missing++){
   B m;assert(m.Arm(nonce,100));
   if(missing!=0)assert(m.Observe(B::Ready,nonce,110,115));
   if(missing!=1)assert(m.Observe(B::Start,nonce,120,125));
   if(missing!=2)assert(m.Observe(B::Done,0,130,135));
   if(missing!=3)assert(m.Observe(B::Payload,nonce,140,145));
   assert(m.Observe(B::Done,nonce,150,155));
   if(missing!=4)assert(m.Observe(B::Payload,~nonce,160,165));
   R result;m.Finish(1000,1000,result);assert(result.state==2 && !result.elapsedMinUs && !result.elapsedMaxUs);unavailable++;
  }
  // No end can be inferred from pristine zero,including nonce FFFFFFFF.
  B early;assert(early.Arm(nonce,100));
  assert(early.Observe(B::Payload,~nonce,101,102));
  R result;early.Finish(1000,1000,result);assert(result.state==2);unavailable++;
 }
 for(unsigned fault=0;fault<12;fault++){
  B b;R r;bool okay=true;
  if(fault==0)okay=b.Arm(0,100);
  else if(fault==1)okay=b.Arm(1,0);
  else if(fault==2)okay=b.Observe(B::Ready,1,100,101);
  else{
   assert(b.Arm(1,100));
   if(fault==3)okay=b.Arm(1,101);
   if(fault==4)okay=b.Observe(B::Ready,1,99,101);
   if(fault==5)okay=b.Observe(B::Ready,1,101,100);
   if(fault==6)okay=b.Observe(B::Ready,1,101,UINT64_MAX);
   if(fault==7)okay=b.Observe(4,1,101,102);
   if(fault==8)okay=b.Observe(B::Done,2,101,102);
   if(fault==9){assert(b.Observe(B::Start,1,101,102));okay=b.Observe(B::Start,0,103,104);}
   if(fault==10){assert(b.Observe(B::Payload,1,101,102));okay=b.Observe(B::Payload,0,103,104);}
   if(fault==11){b=Good(1);okay=b.Observe(B::Payload,1,250,260);}
  }
  assert(!okay);b.Finish(1000,1000,r);assert(r.state==3 && !r.elapsedMinUs && !r.elapsedMaxUs);refused++;
 }
 // Missing/nonpositive separation and zero counter/bytes are unavailable,
 // never a zero denominator or an infinite/firmware-derived rate.
 for(unsigned fault=0;fault<4;fault++){
  auto b=Good(1);R r;
  if(fault<2)b.Finish(fault?1000:0,fault?0:1000,r);
  else{
   B overlap;assert(overlap.Arm(1,100));
   assert(overlap.Observe(B::Ready,1,101,102));
   assert(overlap.Observe(B::Start,1,103,104));
   assert(overlap.Observe(B::Done,0,fault==2?104:105,105));
   assert(overlap.Observe(B::Payload,1,106,107));
   assert(overlap.Observe(B::Done,1,108,109));
   assert(overlap.Observe(B::Payload,~1U,110,111));overlap.Finish(1000,1000,r);
  }
  assert(r.state==2 && !r.elapsedMinUs && !r.elapsedMaxUs);unavailable++;
 }
 printf("PASS_COHORT_HOST_BRACKETS valid%u unavailable%u refused%u nonceExtremes1 microsecondCensor1 missingNeverInvented1;SYNTHETIC_HOST_OBSERVATIONS_NOT_PHYSICAL_FREQUENCY_BANDWIDTH_OR_WORK\n",valid,unavailable,refused);
}
