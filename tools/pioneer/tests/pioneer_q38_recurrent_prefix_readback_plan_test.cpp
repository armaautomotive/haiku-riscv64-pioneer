/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixReadbackPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
int main(){
 namespace L=PioneerQ38MixedRecurrentBlockRootLayout;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(canonical->Build() && other->Build());std::array<uint32_t,1920> sources;
 for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 unsigned geometry=0,refused=0;
 for(unsigned bank=0;bank<7;bank++){
  auto prefix=std::make_unique<PioneerQ38RecurrentPrefixPlan>();assert(prefix->Build(*canonical,bank,{1,2,3,4},sources.data()));
  PioneerQ38RecurrentPrefixReadbackPlan readback;PioneerQ38RecurrentPrefixReadbackPage page,initial;
  assert(!readback.PageCount() && !readback.Page(0,page) && !memcmp(&page,&initial,sizeof(page)));
  assert(readback.Build(*canonical,*prefix) && readback.PageCount()==5 && !readback.Build(*canonical,*prefix));
  const auto* last=prefix->Layer(2)->Outer();assert(last[3]==2 && last[48]==3 && !last[4] && !last[12]);
  for(unsigned i=0;i<5;i++){
   assert(readback.Page(i,page) && page.bank==bank && page.route==PioneerQ38ProjectionRoute(bank) && page.words==1024
    && page.firstWord==i*1024 && page.address==prefix->Workspace()+L::FinalOffset+i*4096);
   geometry++;
  }
  const auto before=page;
  for(unsigned bad:{5U,UINT32_MAX}){assert(!readback.Page(bad,page) && !memcmp(&page,&before,sizeof(page)));refused++;}
  auto reject=[&]{assert(!readback.Page(0,page) && !memcmp(&page,&before,sizeof(page)));refused++;};
  // Mutations in ANY earlier layer must invalidate the LAST-layer request.
  for(unsigned layer=0;layer<3;layer++){
   auto* outer=const_cast<uint32_t*>(prefix->Layer(layer)->Outer());
   for(unsigned word=0;word<128;word++){outer[word]^=1;reject();outer[word]^=1;}
   for(unsigned recipe=0;recipe<8;recipe++){
    auto* data=const_cast<uint32_t*>(prefix->Layer(layer)->Recipe(recipe));
    for(unsigned word:{0U,16U,32U,48U,80U,1279U}){data[word]^=1;reject();data[word]^=1;}
   }
  }
  for(unsigned worker:{0U,119U}){
   auto* boot=const_cast<uint32_t*>(prefix->Boot(worker));
   for(unsigned word=0;word<64;word++){boot[word]^=1;reject();boot[word]^=1;}
  }
  for(void* p:{static_cast<void*>(&readback),static_cast<void*>(canonical.get()),static_cast<void*>(prefix.get()),
   reinterpret_cast<void*>(UINTPTR_MAX-3)}){
   assert(!readback.Page(0,*reinterpret_cast<PioneerQ38RecurrentPrefixReadbackPage*>(p)));refused++;
  }
  assert(prefix->Matches(*canonical) && readback.Page(0,page));
  PioneerQ38RecurrentPrefixReadbackPlan wrong;
  assert(!wrong.Build(*other,*prefix) && !wrong.PageCount() && !wrong.Build(*canonical,*prefix));refused+=2;
 }
 auto cold=std::make_unique<PioneerQ38RecurrentPrefixPlan>();PioneerQ38RecurrentPrefixReadbackPlan unready;
 assert(!unready.Build(*canonical,*cold) && !unready.PageCount());refused++;
 printf("PASS DISTINCT prefix LAST-layer2/request3/token0/fivefixed4096B readback DATA pages%u/all7banks/ALL3layers+recipes+boot+canonical+alias+range atomic refusals%u;NO source,STOP,hold,native,CPU or Work grant\n",geometry,refused);
}
