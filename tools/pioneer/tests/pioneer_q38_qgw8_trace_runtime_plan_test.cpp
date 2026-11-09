/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_QGW8_TRACE_INIT_DATA_INSPECTION 1
#include "../PioneerQ38Qgw8TraceRuntimePlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
using P=PioneerQ38Qgw8TraceRuntimePlan;using B=PioneerQ38TokenRuntimePlan;
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources={};for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540001+i;
 const PioneerQ38GdnIdentity id={UINT64_C(1)<<40|1,UINT64_C(2)<<40|2,UINT64_C(3)<<40|3,UINT64_C(4)<<40|4};
 const uint32_t prompt[]={248319,17};std::array<uint8_t,16384> output={};unsigned chunks=0,refused=0;
 for(unsigned bank=0;bank<7;bank++){
  auto startup=std::make_unique<PioneerQ38TokenStartupPlan>();auto plan=std::make_unique<P>();auto legacy=std::make_unique<B>();
  assert(!plan->Read(0,output.data(),output.size()) && startup->Build(*canonical,bank,id,sources.data(),prompt,2,3,false,248318,248317)
   && plan->Bind(*canonical,*startup,id,sources.data()) && legacy->Bind(*canonical,*startup,id,sources.data())
   && plan->MatchesBinding(*canonical,*startup,id,sources.data()));
  assert(!plan->Bind(*canonical,*startup,id,sources.data()) && plan->Bank()==bank && plan->Workspace()==legacy->Workspace());
  const auto* region=canonical->Workspace(bank);assert(region && region->allocated==0x4000000 && plan->Workspace()==canonical->Base+region->offset);
  std::vector<unsigned char> visited(0x4000000/64);uint64_t total=0;
  for(unsigned n=0;n<P::Chunks;n++){
   P::Chunk d={};assert(plan->Describe(n,d) && d.bank==bank && d.bytes && d.bytes<=16384 && !((d.address|d.bytes)&63));
   assert(d.address>=plan->Workspace() && d.address<=plan->Workspace()+0x4000000-d.bytes);
   for(unsigned j=0;j<d.bytes/64;j++)assert(++visited.at((d.address-plan->Workspace())/64+j)==1);
   if(n<B::Chunks){B::Chunk old={};assert(legacy->Describe(n,old) && !std::memcmp(&d,&old,sizeof(d)));}
   else{
    assert(d.kind==7 && d.offset==(n-B::Chunks)*16384 && d.address==plan->Workspace()+0x3500000+d.offset);
    output.fill(0xa5);assert(plan->Read(n,output.data(),d.bytes));for(auto value:output)assert(!value);
   }
   total+=d.bytes;chunks++;
  }
  assert(P::Chunks==2388 && total==39093248 && P::PayloadBytes==total);
  P::Chunk d={1,2,3,4,5,6},before=d;assert(!plan->Describe(P::Chunks,d) && !std::memcmp(&d,&before,sizeof(d)));
  output.fill(0x5a);const auto saved=output;assert(!plan->Read(B::Chunks,output.data(),16383) && output==saved);
  for(void* bad:{static_cast<void*>(plan.get()),static_cast<void*>(startup.get()),static_cast<void*>(canonical.get()),
                 static_cast<void*>(const_cast<uint32_t*>(PioneerQ38TokenRope::Data))}){
   assert(!plan->Read(B::Chunks,static_cast<uint8_t*>(bad),16384));refused++;
  }
  auto wrong=id;wrong.driverGeneration++;assert(!plan->MatchesBinding(*canonical,*startup,wrong,sources.data()));
  sources[1919]^=1;assert(!plan->MatchesBinding(*canonical,*startup,id,sources.data()));sources[1919]^=1;
 }
 printf("PASS trace runtime DATA %u disjoint chunks/all7 banks, 80 zero slots each, %u alias refusals; NO IO OR OWNERSHIP\n",chunks,refused);
}
