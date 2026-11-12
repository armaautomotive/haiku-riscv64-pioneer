/* SPDX-License-Identifier: MIT */
// Synthetic count/timestamp DATA only; no model,driver or actual rate.
#include "../PioneerQ38TokenRateSamples.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
namespace P=PioneerQ38TokenFrontierControl;
static bool Same(const PioneerQ38TokenRateSamples::Bounds& a,const PioneerQ38TokenRateSamples::Bounds& b){
 return a.prompts==b.prompts && a.limit==b.limit && a.firstCommitted==b.firstCommitted && a.lastCommitted==b.lastCommitted
  && a.sampledTokens==b.sampledTokens && a.minElapsedUs==b.minElapsedUs && a.maxElapsedUs==b.maxElapsedUs
  && a.lowerTokensPerSecond==b.lowerTokensPerSecond && a.upperTokensPerSecond==b.upperTokensPerSecond;
}
static P::Packet Packet(unsigned count,unsigned limit=34,unsigned prompts=2){
 P::Packet p;auto& r=p.reply;r.version=1;r.operation=P::Observe;r.started=r.ready=1;
 r.detail[0]=r.detail[1]=r.detail[6]=1;r.detail[2]=12;r.detail[4]=63;
 r.detail[7]=count;r.detail[8]=limit;r.detail[9]=prompts;assert(P::ReplyShape(p));return p;
}
int main(){
 using S=PioneerQ38TokenRateSamples;unsigned positives=0,refusals=0;
 {
  S s(1,17);S::Bounds b;
  assert(s.ObserveCount(0,10,20) && s.ObserveCount(1,30,40) && s.ObserveCount(1,50,60)
   && s.ObserveCount(17,1000,1010) && s.ObservedBounds(b));
  assert(b.firstCommitted==1 && b.lastCommitted==17 && b.sampledTokens==16
   && b.minElapsedUs==960 && b.maxElapsedUs==980);
  S bad(1,17);assert(bad.ObserveCount(2,10,20) && !bad.ObserveCount(1,30,40) && !bad.ObserveCount(17,50,60));
  S over(1,17);assert(!over.ObserveCount(18,10,20));
 }
 {
  S s(2,34);S::Bounds b;const auto original=b;
  S::Window window;
  assert(!s.ObservedBounds(b) && Same(b,original) && !s.ObservedWindow(window));
  assert(s.Observe(Packet(0),0,10) && s.Observe(Packet(1),20,30) && !s.ObservedBounds(b));
  assert(s.Observe(Packet(2),100,110) && s.Observe(Packet(2),150,160));
  assert(s.Observe(Packet(17),400,410) && !s.ObservedBounds(b));
  assert(s.Observe(Packet(34),900,910) && s.ObservedBounds(b));
  assert(b.prompts==2 && b.limit==34 && b.firstCommitted==2 && b.lastCommitted==34 && b.sampledTokens==32
   && b.minElapsedUs==790 && b.maxElapsedUs==810
   && b.lowerTokensPerSecond==32000000.0/810 && b.upperTokensPerSecond==32000000.0/790);
  assert(s.ObservedWindow(window) && window.firstBegin==100 && window.firstEnd==110
   && window.lastBegin==900 && window.lastEnd==910 && window.firstCount==2 && window.lastCount==34);
  const auto before=b;assert(s.Observe(Packet(34),10000,10010) && s.ObservedBounds(b)
   && Same(b,before));positives++;
  S::Window after;assert(s.ObservedWindow(after) && after.firstBegin==window.firstBegin
   && after.firstEnd==window.firstEnd && after.lastBegin==window.lastBegin && after.lastEnd==window.lastEnd);
 }
 {
  S s(2,34);S::Bounds b;assert(s.Observe(Packet(3),100,120) && s.Observe(Packet(34),1000,1020)
   && s.ObservedBounds(b) && b.firstCommitted==3 && b.sampledTokens==31);positives++;
 }
 for(unsigned start=2;start<34;start++){
  S s(2,34);S::Bounds b;
  assert(s.Observe(Packet(start),1000,1100) && s.Observe(Packet(34),10000,10100) && s.ObservedBounds(b)
   && b.sampledTokens==34-start && b.minElapsedUs==8900 && b.maxElapsedUs==9100);positives++;
 }
 {
  S s(2,34);S::Bounds b;const int64_t end=std::numeric_limits<int64_t>::max();
  assert(s.Observe(Packet(2),0,1) && s.Observe(Packet(34),end-1,end) && s.ObservedBounds(b)
   && b.minElapsedUs==end-2 && b.maxElapsedUs==end && b.lowerTokensPerSecond>0);positives++;
 }
 for(unsigned mode=0;mode<11;mode++){
  S s(2,34);S::Bounds b;assert(s.Observe(Packet(2),100,110));auto p=Packet(3);
  int64_t begin=120,end=130;
  switch(mode){
   case 0:p=Packet(1);break;case 1:begin=109;break;case 2:begin=-1;break;
   case 3:end=119;break;case 4:p.reply.ready=0;break;case 5:p=Packet(3,35);break;
   case 6:p=Packet(3,34,3);break;case 7:p.reply.detail[7]=35;break;
   case 8:p.reply.detail[10]=1;break;case 9:p.reply.status=-1;break;case 10:p.reply.unsafe=1;break;default:assert(false);
  }
  assert(!s.Observe(p,begin,end) && !s.Observe(Packet(34),1000,1010) && !s.ObservedBounds(b));refusals++;
 }
 for(unsigned mode=0;mode<5;mode++){
  const unsigned prompts=mode==0?0:mode==1?34:mode==2?35:2,limit=mode==3?8193:mode==4?0:34;
  S s(prompts,limit);S::Bounds b;assert(!s.Observe(Packet(2),100,110) && !s.ObservedBounds(b));refusals++;
 }
 {S s(2,34);S::Bounds b;assert(s.Observe(Packet(34),100,110) && !s.ObservedBounds(b));refusals++;}
 {S s(2,34);S::Bounds b;assert(s.Observe(Packet(2),100,110) && s.Observe(Packet(34),110,120) && !s.ObservedBounds(b));refusals++;}
 printf("PASS host commit-count bounds/%u synthetic windows/%u clock+counter+packet+config+incomplete refusals/prompts excluded/first post-START count may exceed2/repeats do not shift endpoints/count delta exact/monotonic regression poisons tracker/no signed overflow/never CSR or frequency;DATA arithmetic ONLY,NOT genuine clean END,source,driver,model,physical or measured TPS proof\n",positives,refusals);
}
