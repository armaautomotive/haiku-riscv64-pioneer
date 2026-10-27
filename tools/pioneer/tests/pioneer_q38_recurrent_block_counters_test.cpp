/* SPDX-License-Identifier: MIT */
// Independent packet-by-packet metadata observer. NO hardware/instructions
// executed here;actual emitted-worker correlation remains a separate gate.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38MixedRecurrentBlockCounters.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
static void Packet(std::array<uint32_t,8>& counts,bool write,unsigned bytes){
 counts[write?4:0]++;counts[write?5:1]+=(bytes+63)/64;counts[write?7:3]++;counts[2]++;counts[6]++;
}
static std::array<uint32_t,8> Observe(const PioneerQ38MixedRecurrentBlockPlan& plan,unsigned worker,unsigned polls,bool beforeDown){
 std::array<uint32_t,8> counts={};
 for(unsigned slot=0;slot<(beforeDown?7U:8U);slot++){
  if(slot==4){
   for(unsigned job=0;job<PioneerQ38ConvBatchCount(worker);job++){
    Packet(counts,false,64);Packet(counts,false,256);Packet(counts,false,192);Packet(counts,true,192);Packet(counts,true,64);
   }
   if(worker<=96){
    for(unsigned n:{512U,512U,256U,64U,64U,64U,64U})Packet(counts,false,n);
    Packet(counts,false,16384);Packet(counts,false,16384);Packet(counts,true,16384);Packet(counts,true,16384);
    for(unsigned i=0;i<polls;i++)Packet(counts,false,64);
    Packet(counts,false,256);Packet(counts,false,512);Packet(counts,false,512);Packet(counts,true,256);
   }
  }
  const auto* d=plan.Recipe(slot);if(worker-1>=d[7])continue;
  unsigned bytes=d[14]*2;
  for(unsigned at=0;at<bytes;at+=16384)Packet(counts,false,bytes-at>16384?16384:bytes-at);
  unsigned block=d[15]==8?32:256;Packet(counts,false,((d[14]/block)*4+63)&~63U);
  const auto* p=d+80+(worker-1)*8;
  for(unsigned group=0;group<p[2];group++)for(unsigned k=0;k<d[14];k+=block)Packet(counts,false,d[15]==8?576:2304);
 }
 return counts;
}
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 unsigned cases=0,refusals=0;
 for(unsigned layer:{0U,62U}){
  auto plan=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
  unsigned bank=layer?6:0,address=canonical->Base+canonical->Workspace(bank)->offset+0x600000;
  assert(plan->Build(*canonical,layer,layer?8191:0,248319,bank,layer?1:0,1001,1,{1,2,3,4},sources.data(),bank,address));
  for(unsigned worker=1;worker<120;worker++)for(bool beforeDown:{false,true})for(unsigned trial:{0U,1U}){
   unsigned polls=worker<=96?(trial?256:1):0;std::array<uint32_t,8> delta;
   assert(PioneerQ38RecurrentBlockCounterDelta(*plan,*canonical,worker,polls,beforeDown,delta.data())
    && delta==Observe(*plan,worker,polls,beforeDown));
   std::array<uint32_t,8> before,after;for(unsigned i=0;i<8;i++)before[i]=0xfffffff0U-i*17;before[6]=before[2];
   for(unsigned i=0;i<8;i++)after[i]=before[i]+delta[i];unsigned inferred=~0U;
   assert(PioneerQ38RecurrentBlockCounterProof(*plan,*canonical,worker,before.data(),after.data(),beforeDown,inferred) && inferred==polls);cases++;
   for(unsigned i=0;i<8;i++){
    auto bad=after;bad[i]^=1;inferred=0xa5a5a5a5;
    assert(!PioneerQ38RecurrentBlockCounterProof(*plan,*canonical,worker,before.data(),bad.data(),beforeDown,inferred) && inferred==0xa5a5a5a5);refusals++;
   }
  }
  std::array<uint32_t,8> sentinel;sentinel.fill(0xa5a5a5a5);
  for(unsigned worker:{0U,120U,~0U}){
   assert(!PioneerQ38RecurrentBlockCounterDelta(*plan,*canonical,worker,1,false,sentinel.data()));refusals++;
  }
  for(auto bad:std::array<std::pair<unsigned,unsigned>,4>{{{1,0},{96,257},{97,1},{119,256}}}){
   assert(!PioneerQ38RecurrentBlockCounterDelta(*plan,*canonical,bad.first,bad.second,false,sentinel.data()));refusals++;
  }
  auto* recipe=const_cast<uint32_t*>(plan->Recipe(0));recipe[43]^=1;
  assert(!PioneerQ38RecurrentBlockCounterDelta(*plan,*canonical,1,1,false,sentinel.data()));recipe[43]^=1;refusals++;
  for(auto word:sentinel)assert(word==0xa5a5a5a5);
 }
 printf("PASS QRB1 complete-layer counter DATA packet-by-packet cases%u/atomicRefusals%u/ALL119/small3/Conv39-40/GDN96-97/polls1+256/beforeDown+final/wrapped baselines;NOT actual instructions/native IO/STOP/Work/numeric authority\n",cases,refusals);
}
