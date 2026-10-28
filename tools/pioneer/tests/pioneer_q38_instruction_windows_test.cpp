/* SPDX-License-Identifier: MIT */
#include "PioneerQ38InstructionWindows.h"
#include "PioneerQ38InstructionProfile.h"
#include <cassert>
#include <cstdio>
#include <cstring>
template<unsigned Bound> static void MoreWindows(){
 using W=PioneerQ38InstructionWindowSet<Bound>;
 for(unsigned low=0;low<256;low++){
  W w;uint64_t total=0;unsigned steps=0;
  for(unsigned layer=1;layer<=Bound;layer++){
   unsigned added=700000000U+(low*137+layer*19)%1000000;steps+=added;total+=added;unsigned tick=steps&255;
   assert(w.Total(steps)==total && w.RebaseAfterVerifiedLayer(steps,layer) && w.Total(steps)==total && steps==tick && w.Closed()==layer);
  }
  const auto before=w;unsigned old=steps;assert(total>UINT32_MAX && !w.RebaseAfterVerifiedLayer(steps,Bound+1) && steps==old && before.Total(old)==w.Total(steps));
 }
}
int main(int argc,char** argv){
 assert(argc==2);
 using W=PioneerQ38InstructionWindows;unsigned refusals=0,cases=0;
 for(unsigned low=0;low<256;low++){
  W w;uint64_t total=0;unsigned steps=0;
  for(unsigned layer=1;layer<=64;layer++){
   unsigned added=700000000U+(low*137+layer*19)%1000000;steps+=added;total+=added;
   unsigned before=steps,tick=steps&255;
   assert(w.Total(steps)==total && w.RebaseAfterVerifiedLayer(steps,layer) && w.Total(steps)==total && steps==tick && steps<=255 && w.Closed()==layer);
   for(unsigned distance=0;distance<768;distance++)assert(((before+distance)&255)==((steps+distance)&255));
   cases++;
  }
  assert(total>UINT32_MAX && w.Total(steps)==total && total<W::TotalLimit);
  auto saved=w;unsigned before=steps;assert(!w.RebaseAfterVerifiedLayer(steps,65) && steps==before && w.Total(steps)==saved.Total(before));refusals++;
 }
 for(unsigned bad=0;bad<4;bad++){
  W w;unsigned steps=bad<2?0:bad==2?W::Limit:UINT32_MAX;unsigned count=bad==0?0:bad==1?2:1;
  auto saved=w;unsigned before=steps;assert(!w.RebaseAfterVerifiedLayer(steps,count) && steps==before && w.Total(steps)==saved.Total(before) && !w.Closed());refusals++;
 }
 W w;unsigned steps=W::Limit-1;assert(w.RebaseAfterVerifiedLayer(steps,1) && w.Total(steps)==W::Limit-1);
 unsigned residue=steps;assert(residue);
 steps=residue-1;uint64_t before=w.Total(steps);assert(!w.RebaseAfterVerifiedLayer(steps,2) && w.Total(steps)==before && steps==residue-1);refusals++;
 printf("PASS OFFLINE ROOT observation windows/cases%u/refusals%u/64verified-stage bookkeeping/64bit exact totals beyond UINT32/preserve ALL low8 tick phases+next768/unchanged4B per-window bound+bounded total/no state changes on refusal;NO PC,register,NoC,Tensix,clock,product or native/token authority\n",cases,refusals);
 PioneerQ38InstructionProfile profile(argv[1]);
 const unsigned pcs[]={0x40000,0x40004,0x40000,0x40008,0x5fffc,0x64000,0x6bffc,0x6000,0x60000,0x60004};
 for(unsigned pc:pcs)profile.Record(pc);
 assert(profile.Total()==10);profile.Dump(10);
 MoreWindows<192>();
 using Full=PioneerQ38InstructionWindowSet<64*8192>;Full full;unsigned n=0;uint64_t total=0;
 for(unsigned stage=1;stage<=Full::Layers;stage++){n+=700000000U;total+=700000000U;assert(full.RebaseAfterVerifiedLayer(n,stage) && full.Total(n)==total);}
 assert(!full.RebaseAfterVerifiedLayer(n,Full::Layers+1));
 puts("PASS extended OFFLINE observation192 stages x256 tick phases/full524288-stage arithmetic bound/unchanged4B windows;NO device/model-state authority");
}
