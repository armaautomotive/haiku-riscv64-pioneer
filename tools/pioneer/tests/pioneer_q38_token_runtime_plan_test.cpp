/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38TokenRuntimePlan.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);unsigned refusals=0,chunks=0;
 // Independent raw-configuration calculation ONLY,not inference outputs.
 for(unsigned p=0;p<8192;p++)for(unsigned i=0;i<32;i++){
  float angle=float(p)*powf(10000000.f,-float(i)/32.f);const float expected[]={cosf(angle),sinf(angle)};
  for(unsigned which=0;which<2;which++){uint32_t bits;memcpy(&bits,&expected[which],4);assert(PioneerQ38TokenRope::Data[p*64+i*2+which]==bits);}
 }
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto foreign=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(canonical->Build() && foreign->Build());std::array<uint32_t,1920> sources;
 for(unsigned i=0;i<1920;i++)sources[i]=0x76540001+i;
 const PioneerQ38GdnIdentity id={UINT64_C(1)<<40|1,UINT64_C(2)<<40|2,UINT64_C(3)<<40|3,UINT64_C(4)<<40|4};
 const uint32_t prompt[]={248319,17};std::array<uint8_t,16384> output;
 for(unsigned bank=0;bank<7;bank++){
  auto startup=std::make_unique<PioneerQ38TokenStartupPlan>();auto plan=std::make_unique<PioneerQ38TokenRuntimePlan>();
  assert(!plan->Read(0,output.data(),output.size()));
  assert(startup->Build(*canonical,bank,id,sources.data(),prompt,2,3,false,248318,248317)
   && plan->Bind(*canonical,*startup,id,sources.data()) && plan->MatchesBinding(*canonical,*startup,id,sources.data()));
  assert(!plan->Bind(*canonical,*startup,id,sources.data()) && !plan->MatchesBinding(*foreign,*startup,id,sources.data()));
  std::vector<uint8_t> visited(67108864/64);unsigned total=0,kinds[7]={};
  for(unsigned n=0;n<PioneerQ38TokenRuntimePlan::Chunks;n++){
   PioneerQ38TokenRuntimePlan::Chunk d;assert(plan->Describe(n,d) && d.bank==bank && d.kind>=1 && d.kind<=6 && d.bytes<=16384
    && !(d.address&63) && !(d.bytes&63) && d.address>=plan->Workspace() && d.address<=plan->Workspace()+67108864-d.bytes
    && plan->Read(n,output.data(),d.bytes));
   for(unsigned i=0;i<d.bytes/64;i++)assert(++visited.at((d.address-plan->Workspace())/64+i)==1);
   if(d.kind==2)for(unsigned i=0;i<d.bytes;i++)assert(output[i]==uint8_t(PioneerQ38TokenRope::Data[(d.offset+i)/4]>>(((d.offset+i)&3)*8)));
   if(d.kind==6)for(unsigned i=0;i<d.bytes;i++)assert(!output[i]);
   output.fill(0x5a);const auto before=output;
   assert(!plan->Read(n,output.data(),d.bytes-1) && output==before);refusals++;
   total+=d.bytes;kinds[d.kind]++;chunks++;
  }
  assert(total==37782528 && kinds[1]==2048 && kinds[2]==128 && kinds[3]==1 && kinds[4]==1 && kinds[5]==2 && kinds[6]==128);
  for(unsigned bad=0;bad<4;bad++){
   auto copy=id;if(bad==0)copy.session^=UINT64_C(1)<<63;if(bad==1)copy.sequence++;
   if(bad==2)copy.modelEpoch++;if(bad==3)copy.driverGeneration++;
   assert(!plan->MatchesBinding(*canonical,*startup,copy,sources.data()));refusals++;
  }
  for(unsigned i=0;i<1920;i++){sources[i]^=1;assert(!plan->MatchesBinding(*canonical,*startup,id,sources.data()));sources[i]^=1;refusals++;}
  for(unsigned n:{0U,2048U,2176U,2177U,2178U,2180U,2307U}){
   PioneerQ38TokenRuntimePlan::Chunk d;assert(plan->Describe(n,d));
   for(void* bad:{static_cast<void*>(plan.get()),static_cast<void*>(startup.get()),static_cast<void*>(canonical.get()),
    static_cast<void*>(const_cast<uint32_t*>(PioneerQ38TokenRope::Data))}){assert(!plan->Read(n,static_cast<uint8_t*>(bad),d.bytes));refusals++;}
  }
  PioneerQ38TokenRuntimePlan::Chunk bad={1,2,3,4,5,6},saved=bad;output.fill(0x5a);const auto before=output;
  assert(!plan->Describe(2308,bad) && !memcmp(&bad,&saved,sizeof(bad)) && !plan->Read(2308,output.data(),64) && output==before);
  printf("PASS runtime DATA bank%u/ALL2308 disjoint chunks/37782528B/full32MiB packages+2MiB rawRoPE+head+session+prompt+2MiB zero records/NO KV-row initialization\n",bank);
 }
 printf("PASS runtime DATA all7banks/chunks%u/ALL524288 rawRoPE words independently recomputed/high64 identity+all1920 source changes/aliases+range+size refusals%u;NO native/IO/model/Work/TPS grant\n",chunks,refusals);
}
