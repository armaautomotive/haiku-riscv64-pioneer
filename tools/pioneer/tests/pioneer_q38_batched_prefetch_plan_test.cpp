/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "../PioneerQ38BatchedPrefetchPlan.h"
#include "../PioneerQ38Batch4Fetch.h"
using Plan=PioneerQ38BatchedPrefetchPlan;
static unsigned checked=0,refused=0;
static void Deny(unsigned type,unsigned inputs,unsigned groups,unsigned group,unsigned block,uint32_t first){
 Plan p;p.currentSource=1;p.currentRaw=2;p.copySource=3;p.tileBytes=4;p.bulkSource=5;
 p.bulkBytes=6;p.nextSource=7;p.first=p.batchFirst=p.copyLast=p.next=true;
 const Plan saved=p;
 assert(!Plan::Describe(type,inputs,groups,group,block,first,p));
 assert(!memcmp(&p,&saved,sizeof(p)));refused++;
}
int main(){
 static_assert(sizeof(Plan)==32,"bounded pure DATA only");
 for(unsigned type:{8U,12U,14U})for(unsigned inputs:{5120U,6144U,17408U})for(unsigned groups=1;groups<=16;groups++){
  const unsigned tile=Plan::Tile(type),batch=Plan::Batch(type),blocks=inputs>>(type==8?5:8);
  assert(tile==(type==8?576U:type==12?2304U:3392U));
  assert(batch==(type==8?4U:2U) && Plan::ReadBlocks(type,inputs,false)==blocks
   && Plan::ReadBlocks(type,inputs,true)*batch==blocks);
  for(uint32_t first:{0x04000000U,0xc4000000U-groups*blocks*tile}){
   unsigned packets=0,bytes=0,copies=0;
   for(unsigned group=0;group<groups;group++)for(unsigned block=0;block<blocks;block++){
    Plan p;PioneerQ38Batch4FetchPlan old;
    assert(Plan::Describe(type,inputs,groups,group,block,first,p)
     && PioneerQ38Batch4Fetch(type,inputs,groups,group,block,first,old));
    assert(p.bulkSource==old.source && p.bulkBytes==old.bytes && p.copySource==old.copySource
     && p.tileBytes==old.copyWords*4 && p.batchFirst==old.issue);
    assert(p.currentSource==first+(group*blocks+block)*tile && p.first==(block==0)
     && p.copyLast==(block%batch==batch-1) && p.next==(p.copyLast && block+1<blocks)
     && p.currentRaw==(p.copyLast?0x90000U:p.copySource));
    assert(p.bulkBytes<=8192 && 0x98000+p.bulkBytes<=0x99a80 && p.copySource>=0x98000
     && p.copySource+tile<=0x98000+p.bulkBytes && p.bulkSource+p.bulkBytes<=first+groups*blocks*tile
     && Plan::RawInput(type,p.currentRaw) && Plan::RawInput(type,p.copySource));
    if(p.next)assert(p.nextSource==p.bulkSource+p.bulkBytes);else assert(!p.nextSource);
    if(p.first || p.next){packets++;bytes+=p.bulkBytes;}copies+=p.copyLast;checked++;
   }
   assert(packets==groups*(blocks/batch) && bytes==groups*blocks*tile && copies==packets);
  }
  Deny(type,inputs,groups,groups,0,0x04000000);
  Deny(type,inputs,groups,0,blocks,0x04000000);
  Deny(type,inputs,groups,0,0,0xc4000000-groups*blocks*tile+64);
 }
 for(unsigned type:{8U,12U,14U})for(unsigned inputs:{5120U,6144U,17408U}){
  for(unsigned value:{0U,17U,UINT32_MAX})Deny(type,inputs,value,0,0,0x04000000);
  for(uint32_t first:{0U,0x03ffffc0U,0x04000001U,0x04000020U,0xc4000000U,UINT32_MAX})Deny(type,inputs,1,0,0,first);
  for(unsigned k:{0U,32U,256U,5119U,5121U,6143U,17409U,UINT32_MAX}){
   Deny(type,k,1,0,0,0x04000000);assert(!Plan::ReadBlocks(type,k,false) && !Plan::ReadBlocks(type,k,true));
  }
 }
 for(unsigned type:{0U,7U,9U,13U,15U,UINT32_MAX}){
  Deny(type,5120,1,0,0,0x04000000);assert(!Plan::Batch(type) && !Plan::Tile(type)
   && !Plan::ReadBlocks(type,5120,true) && !Plan::RawInput(type,0x90000));
 }
 for(unsigned type:{8U,12U,14U})for(uint32_t raw=0x8fffc;raw<=0x99a84;raw+=4){
  bool expected=raw==0x90000;
  for(unsigned at=0;at<Plan::Batch(type);at++)expected|=raw==0x98000+at*Plan::Tile(type);
  assert(Plan::RawInput(type,raw)==expected);checked++;
 }
 printf("PASS batched-prefetch pure DATA/%u proposals+raw-input checks/%u unchanged-output refusals/ALLtypes,K,groups,source-end boundaries/old bounded batches exact/unchanged total bytes/fewer read commands/last-tile-only copy/disjoint saved scratch;NO response,drain,owner,execution,hardware or speed proof\n",checked,refused);
}
