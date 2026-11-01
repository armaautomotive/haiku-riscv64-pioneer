/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "../PioneerQ38TwinBatchPrefetchPlan.h"
#include "../PioneerQ38TokenPushedFootprint.h"
using Plan=PioneerQ38TwinBatchPrefetchPlan;
static unsigned checked=0,refused=0;
static void Deny(unsigned type,unsigned inputs,unsigned groups,unsigned group,unsigned block,uint32_t first){
 Plan p;p.currentSource=1;p.currentRaw=2;p.bulkSource=3;p.bulkRaw=4;
 p.bulkBytes=5;p.nextSource=6;p.nextRaw=7;p.tileBytes=8;p.first=p.batchFirst=p.next=p.restore=true;
 const Plan saved=p;
 assert(!Plan::Describe(type,inputs,groups,group,block,first,p));
 assert(!memcmp(&p,&saved,sizeof(p)));refused++;
}
int main(){
 static_assert(sizeof(Plan)==36,"bounded pure geometry only");
 for(unsigned type:{8U,12U,14U})for(unsigned inputs:{5120U,6144U,17408U})for(unsigned groups=1;groups<=16;groups++){
  const unsigned tile=type==8?576:type==12?2304:3392,batch=type==8?4:2,blocks=inputs/(type==8?32:256);
  for(uint32_t first:{0x04000000U,0xc4000000U-groups*blocks*tile}){
   unsigned packets=0,bytes=0,restores=0;
   for(unsigned group=0;group<groups;group++)for(unsigned block=0;block<blocks;block++){
    Plan p;assert(Plan::Describe(type,inputs,groups,group,block,first,p));
    const unsigned buffer=(block/batch)%2?0x9a000:0x98000;
    assert(p.currentSource==first+(group*blocks+block)*tile && p.currentRaw==buffer+(block%batch)*tile
     && p.bulkSource==first+(group*blocks+block-block%batch)*tile && p.bulkRaw==buffer
     && p.bulkBytes==batch*tile && p.tileBytes==tile && p.first==(block==0)
     && p.batchFirst==(block%batch==0) && p.next==(block%batch==0 && block+batch<blocks)
     && p.restore==(block%batch==0 && block+batch==blocks));
    assert(p.bulkRaw+p.bulkBytes<=0x9ba80 && p.bulkBytes<=8192 && p.bulkRaw>=0x98000
     && p.currentRaw+p.tileBytes<=p.bulkRaw+p.bulkBytes && Plan::RawInput(type,p.currentRaw));
    for(unsigned worker:{1U,69U,119U})for(unsigned end:{0U,p.bulkBytes-4})
     assert(PioneerQ38TokenPushedL1::SnapshotContains(worker,p.bulkRaw+end));
    if(p.first){packets++;bytes+=p.bulkBytes;}
    if(p.next){
     assert(p.nextSource==p.bulkSource+p.bulkBytes && p.nextRaw==(buffer==0x98000?0x9a000U:0x98000U)
      && (p.nextRaw+p.bulkBytes<=p.bulkRaw || p.bulkRaw+p.bulkBytes<=p.nextRaw)
      && p.nextSource+p.bulkBytes<=first+(group+1)*blocks*tile);
     packets++;bytes+=p.bulkBytes;
    }else assert(!p.nextSource && !p.nextRaw);
    if(p.restore){assert(p.bulkSource+p.bulkBytes-tile==first+((group+1)*blocks-1)*tile);restores++;}
    checked++;
   }
   assert(packets==groups*(blocks/batch) && bytes==groups*blocks*tile && restores==groups);
  }
  Deny(type,inputs,groups,groups,0,0x04000000);
  Deny(type,inputs,groups,0,blocks,0x04000000);
  Deny(type,inputs,groups,0,0,0xc4000000-groups*blocks*tile+64);
 }
 for(unsigned type:{8U,12U,14U})for(unsigned inputs:{5120U,6144U,17408U}){
  for(unsigned value:{0U,17U,UINT32_MAX})Deny(type,inputs,value,0,0,0x04000000);
  for(uint32_t first:{0U,0x03ffffc0U,0x04000001U,0x04000020U,0xc4000000U,UINT32_MAX})Deny(type,inputs,1,0,0,first);
  for(unsigned k:{0U,32U,256U,5119U,5121U,6143U,17409U,UINT32_MAX})Deny(type,k,1,0,0,0x04000000);
 }
 for(unsigned type:{0U,7U,9U,13U,15U,UINT32_MAX}){
  Deny(type,5120,1,0,0,0x04000000);assert(!Plan::RawInput(type,0x90000));
 }
 for(unsigned type:{8U,12U,14U})for(uint32_t raw=0x8fffc;raw<=0xa0004;raw+=4){
  bool expected=raw==0x90000;
  const unsigned tile=type==8?576:type==12?2304:3392,batch=type==8?4:2;
  for(unsigned buffer:{0x98000U,0x9a000U})for(unsigned at=0;at<batch;at++)expected|=raw==buffer+at*tile;
  assert(Plan::RawInput(type,raw)==expected);checked++;
 }
 printf("PASS twin-batch pure DATA/%u proposals+raw-input checks/%u unchanged-output refusals/ALLtypes,K,groups,source-end boundaries/unchanged bytes+packet counts/two whole batches inside existing saved peer span/no per-tile copy geometry;NO response,drain,execution,hardware or speed proof\n",checked,refused);
}
