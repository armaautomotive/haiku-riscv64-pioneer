/* SPDX-License-Identifier: MIT */
// Synthetic USER observation arithmetic/CLI only;NO native or device IO.
#include "../PioneerQ38TokenFrontendSamples.h"
#include "../PioneerQ38FrontendProfileCli.h"
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 using Mode=PioneerQ38FrontendProfileMode;
 const char* run[]={"profile","--profile-frontends-34-positions",nullptr};
 assert(PioneerQ38FrontendProfileArguments(2,run)==Mode::Positions34);
 const char* source[]={"profile","--preflight-source-only",nullptr};
 assert(PioneerQ38FrontendProfileArguments(2,source)==Mode::SourceOnly);
 unsigned denied=0;
 for(int n:{-1,0,1,3,4,100}){assert(PioneerQ38FrontendProfileArguments(n,run)==Mode::Invalid);denied++;}
 for(const char* text:{"--benchmark-34-positions","--generate-34-positions","--generate-three-positions",
  "--work","--cpu","--help","","--profile-frontends-34-positions=1","--profile-frontends-34-positions ",
  " --profile-frontends-34-positions","--preflight-source-only=1"}){
  const char* bad[]={"profile",text,nullptr};assert(PioneerQ38FrontendProfileArguments(2,bad)==Mode::Invalid);denied++;
 }
 assert(PioneerQ38FrontendProfileArguments(2,nullptr)==Mode::Invalid);denied++;
 PioneerQ38TokenFrontendSnapshot pending;pending.generation=7;pending.stage=9;pending.checkedWord=63;
 pending.firstPC=0xffffffffU;pending.secondPC=0x60008; // RAW,not an address/retirement qualification
 auto done=pending;done.ready=1;done.stage=11;
 auto rows=std::make_unique<PioneerQ38TokenFrontendSamples>();
 assert(!rows->Complete() && !rows->Size() && !rows->Generation());
 assert(rows->Observe(100,110,0,pending) && !rows->Complete());
 assert(rows->Observe(200,210,34,done) && rows->Complete() && rows->Size()==2 && rows->Generation()==7);
 assert(rows->Data()[0].observed.firstPC==0xffffffffU && rows->Data()[0].observed.secondPC==0x60008);
 assert(!rows->Observe(300,310,34,done) && !rows->Complete());denied++;
 for(unsigned fault=0;fault<13;fault++){
  auto f=std::make_unique<PioneerQ38TokenFrontendSamples>();assert(f->Observe(100,110,2,pending));
  auto value=pending;int64_t begin=200,end=210;unsigned count=3;
  if(fault==0)begin=-1;if(fault==1)end=begin-1;if(fault==2)begin=109;if(fault==3)count=1;
  if(fault==4)count=35;if(fault==5)value.generation=0;if(fault==6)value.generation=8;
  if(fault==7)value.ready=2;if(fault==8)value.stage=11;if(fault==9)value.checkedWord=64;
  if(fault==10)value.reserved=1;if(fault==11){value.ready=1;value.stage=9;}
  if(fault==12){value.ready=1;value.stage=11;value.reserved=1;}
  assert(!f->Observe(begin,end,count,value) && !f->Complete() && f->Size()==1);
  assert(!f->Observe(300,310,3,pending));denied++;
 }
 auto full=std::make_unique<PioneerQ38TokenFrontendSamples>();
 for(unsigned i=0;i<PioneerQ38TokenFrontendSamples::Capacity;i++)assert(full->Observe(i*2,i*2+1,0,pending));
 assert(!full->Observe(40000,40001,0,done) && !full->Complete());denied++;
 printf("PASS frontend USER DATA/CLI positives/rawFFFFFFFF+60008 preserved/%u time,count,generation,stage,reserved,capacity,late+CLI refusals;NO IO,retiredPC,cycle,model,native,physical or Work proof\n",denied);
}
