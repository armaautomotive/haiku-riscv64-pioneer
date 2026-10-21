/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_WORK_TILE_PREFETCH
#include "../PioneerQ38TilePrefetchPlan.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
static bool Separate(unsigned a,unsigned n,unsigned b,unsigned m){return uint64_t(a)+n<=b||uint64_t(b)+m<=a;}
int main(){
 unsigned cases=0;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U}){
  unsigned bytes=type==8?576:type==12?2304:3392,blocks=k/(type==8?32:256);
  uint32_t total=groups*blocks*bytes;
  for(uint32_t first:{0x04000000U,0xc4000000U-total})for(unsigned group=0;group<groups;group++)for(unsigned block=0;block<blocks;block++){
   PioneerQ38TilePrefetchPlan p;assert(PioneerQ38TilePrefetchPlan::Describe(type,k,groups,group,block,first,p));
   assert(p.bytes==bytes&&p.currentSource==first+(group*blocks+block)*bytes&&p.currentRaw==unsigned(block%2?0x98000:0x90000));
   assert(p.next==(block+1<blocks));
   if(p.next)assert(p.nextSource==p.currentSource+bytes&&p.nextRaw==unsigned(block%2?0x90000:0x98000)&&Separate(p.currentRaw,bytes,p.nextRaw,bytes));
   else assert(!p.nextSource&&!p.nextRaw);
   // Already-saved raw views are disjoint from live code/control/proof,
   // FP32 affine/accumulator,input-sum cache and activation operands.
   for(unsigned raw:{p.currentRaw,p.nextRaw?p.nextRaw:p.currentRaw}){
    assert(!(raw&63)&&bytes<=3392&&Separate(raw,bytes,0x70000,0x4400)&&Separate(raw,bytes,0x83f00,0x100)
     &&Separate(raw,bytes,0x84000,0x400)&&Separate(raw,bytes,0x94000,0x1100)&&Separate(raw,bytes,0x95000,0x1100)
     &&Separate(raw,bytes,0x97000,0x400)&&Separate(raw,bytes,0xa0000,k*2+224));
   }
   cases++;
  }
 }
 for(unsigned fault=0;fault<11;fault++){
  unsigned type=12,k=5120,groups=3,group=0,block=0;uint32_t first=0x04000000;
  switch(fault){case 0:type=0;break;case 1:k=5121;break;case 2:groups=0;break;case 3:groups=17;break;case 4:group=3;break;case 5:block=20;break;
   case 6:first=0x03ffffc0;break;case 7:first++;break;case 8:first=0xc4000000;break;case 9:first=0xffffffc0;break;case 10:first=0xc3ffffc0;break;}
  PioneerQ38TilePrefetchPlan p,old;p.currentRaw=old.currentRaw=0xdeadbeef;
  assert(!PioneerQ38TilePrefetchPlan::Describe(type,k,groups,group,block,first,p)&&p.currentSource==old.currentSource&&p.currentRaw==old.currentRaw
   &&p.nextSource==old.nextSource&&p.nextRaw==old.nextRaw&&p.bytes==old.bytes&&p.next==old.next);
 }
 printf("PASS %u exact original one-tile prefetch DATA proposals,all type/K/group/block/last-boundaries,disjoint original raw buffers,no extra bytes,11 atomic shape/overflow/alias-range refusals;NO IO/completion/source/owner/native authority\n",cases);
}
