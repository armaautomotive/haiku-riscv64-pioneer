/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "../PioneerQ38Batch4Fetch.h"
int main() {
 uint64_t cases=0;
 for(unsigned type:{8U,12U,14U})for(unsigned inputs:{5120U,6144U,17408U})for(unsigned groups=1;groups<=16;groups++) {
  unsigned tile=type==8?576:type==12?2304:3392,blocks=inputs/(type==8?32:256),batch=type==8?4:2;
  uint32_t total=groups*blocks*tile;
  for(uint32_t first:{UINT32_C(0x04000000),UINT32_C(0x04000020),UINT32_C(0xc4000000)-total}) {
   unsigned commands=0;uint64_t fetched=0,consumed=0;
   for(unsigned g=0;g<groups;g++)for(unsigned b=0;b<blocks;b++) {
    PioneerQ38Batch4FetchPlan p;assert(PioneerQ38Batch4Fetch(type,inputs,groups,g,b,first,p));
    assert(p.source==first+(g*blocks+(b/batch)*batch)*tile && p.bytes==batch*tile && p.destination==0x98000 && p.bytes<=8192);
    assert(p.copyWords*4==tile && p.copySource==0x98000+(b%batch)*tile && p.issue==!(b%batch));
    assert(p.source>=first && uint64_t(p.source)+p.bytes<=uint64_t(first)+total);
    // Independent interval observer:each original tile consumed exactly
    // once/in order;four-tile reads partition ONLY initialized slice.
    if(p.issue){assert(p.source==first+fetched);commands++;fetched+=p.bytes;}
    assert(first+consumed==p.source+(p.copySource-p.destination));consumed+=tile;
    assert(p.copySource>=0x98000 && p.copySource+tile<=0x99a80);
    cases++;
   }
   assert(fetched==total && consumed==total && commands*batch==groups*blocks);
  }
  for(unsigned fault=0;fault<10;fault++) {
   unsigned t=type,k=inputs,n=groups,g=0,b=0;uint32_t first=0x04000000;
   switch(fault){case 0:t=0;break;case 1:k=0;break;case 2:n=0;break;case 3:n=17;break;
    case 4:g=groups;break;case 5:b=blocks;break;case 6:first--;break;case 7:first++;break;
    case 8:first=0xc4000000;break;case 9:first=0xc4000000-total+32;break;}
   PioneerQ38Batch4FetchPlan p;p.source=0x12345678;p.destination=0xabcdef00;p.issue=true;auto before=p;
   assert(!PioneerQ38Batch4Fetch(t,k,n,g,b,first,p) && !memcmp(&p,&before,sizeof(p)));cases++;
  }
 }
 printf("PASS %llu OFFLINE bounded batch metadata/source/scratch/overflow/atomic-refusal cases;ALL original bytes once/in order,<=8KiB,75%% Q8/50%% Q4+Q6 fewer modeled requests SAME bytes,NO timing/native/IO/owner authority\n",(unsigned long long)cases);
}
