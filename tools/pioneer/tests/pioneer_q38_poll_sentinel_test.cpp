/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "../PioneerQ38PollSentinel.h"
int main() {
 uint32_t parts[119][8]={};unsigned result=999,cases=0;
 assert(!PioneerQ38SelectPollSentinel(nullptr,1,result) && result==999);
 for(unsigned workers:{0U,120U,UINT32_MAX})assert(!PioneerQ38SelectPollSentinel(parts,workers,result) && result==999);
 uint32_t random=731;
 for(unsigned workers=1;workers<=119;workers++)for(unsigned trial=0;trial<32;trial++) {
  unsigned expected=0;
  for(unsigned w=0;w<workers;w++) {random=random*1664525+1013904223;parts[w][2]=1+(random%16);}
  for(unsigned w=0;w<workers;w++)if(parts[w][2]>=parts[expected][2])expected=w;
  assert(PioneerQ38SelectPollSentinel(parts,workers,result) && result==expected);cases++;
  for(unsigned bad:{0U,17U,UINT32_MAX}) {
   unsigned at=trial%workers,old=parts[at][2];parts[at][2]=bad;result=999;
   assert(!PioneerQ38SelectPollSentinel(parts,workers,result) && result==999);parts[at][2]=old;cases++;
  }
 }
 for(auto& p:parts)p[2]=3;
 assert(PioneerQ38SelectPollSentinel(parts,119,result) && result==118);
 unsigned backoff=64;
 for(unsigned i=0;i<100;i++){unsigned next=PioneerQ38PollBackoffNext(backoff);assert(next>=64 && next<=2048 && next>=backoff);backoff=next;}
 assert(backoff==2048);
 for(unsigned bad:{0U,1U,63U,65U,96U,1023U,2047U,2049U,4096U,UINT32_MAX})assert(!PioneerQ38PollBackoffNext(bad));
 printf("PASS %u fixed119-worker metadata sentinel/max-group/late-tie/atomic refusal cases;bounded backoff;NO completion/IO/address authority\n",cases);
}
