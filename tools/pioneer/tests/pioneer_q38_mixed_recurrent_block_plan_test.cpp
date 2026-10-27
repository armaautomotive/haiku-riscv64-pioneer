/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38MixedRecurrentBlockPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
int main(){
 using C=PioneerQ38CanonicalModelPlan;using P=PioneerQ38MixedRecurrentBlockPlan;
 auto c=std::make_unique<C>(),foreign=std::make_unique<C>();assert(c->Build() && foreign->Build());
 std::array<uint32_t,1920> source;for(unsigned i=0;i<source.size();i++)source[i]=0x16005+i*131;
 std::vector<uint8_t> pages(PioneerResidentBanks::Pages,1);unsigned cases=0,mutations=0;
 for(unsigned layer:{0U,62U})for(unsigned bank:{0U,6U})for(unsigned pos:{0U,8191U})for(unsigned slot:{0U,1U}){
  auto p=std::make_unique<P>();unsigned address=c->Base+c->Workspace(bank)->offset+0x600000;
  assert(p->Build(*c,layer,pos,248319,bank,slot,1001,1,{1,2,3,4},source.data(),bank,address));
  assert(p->Matches(*c) && !p->Matches(*foreign) && p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));cases++;
  assert(!p->Build(*c,layer,pos,248319,bank,slot,1001,1,{1,2,3,4},source.data(),bank,address));
  if(layer || bank || pos || slot)continue;
  auto mutate=[&](const uint32_t* words,unsigned count){for(unsigned i=0;i<count;i++){
   auto* word=const_cast<uint32_t*>(words)+i;*word^=1;assert(!p->Matches(*c));*word^=1;mutations++;
  }assert(p->Matches(*c));};
  mutate(p->Outer(),128);for(unsigned w:{0U,1U,119U})mutate(p->Boot(w),64);
  for(unsigned n=0;n<8;n++)mutate(p->Recipe(n),1280);
  mutate(p->Conv()+119*384,384);mutate(p->Gdn()+95*192,192);
 }
 for(unsigned bad=0;bad<12;bad++){
  auto p=std::make_unique<P>();unsigned address=c->Base+c->Workspace(0)->offset+0x600000;
  assert(!p->Build(*c,bad==0?3:0,bad==1?8192:0,bad==2?248320:0,bad==3?7:0,bad==4?2:0,
   bad==5?0:bad==6?0x7ffffff0:1001,bad==7?0:1,bad==8?PioneerQ38GdnIdentity{0,2,3,4}:PioneerQ38GdnIdentity{1,2,3,4},
   bad==9?nullptr:source.data(),bad==10?7:0,bad==11?address+1:address));
  assert(!p->Outer() && !p->Recipe(0) && !p->Conv() && !p->Gdn() && !p->Boot(0));
 }
 namespace L=PioneerQ38MixedRecurrentBlockRootLayout;
 std::array<uint32_t,128> stop={};stop[0]=L::Magic;stop[1]=1;stop[2]=128;stop[5]=129;
 stop[16]=1;for(unsigned i=32;i<40;i+=2)stop[i]=1;stop[48]=2;stop[49]=119;stop[72]=L::Stop;
 assert(L::StopShape(stop.data()) && !L::OuterShape(stop.data()) && !L::StopShape(nullptr));
 unsigned stopRefusals=0;
 for(unsigned i=0;i<128;i++){
  if(i>=16 && i<40)continue;auto changed=stop;changed[i]^=0x80000000U;
  assert(!L::StopShape(changed.data()));stopRefusals++;
 }
 for(unsigned value:{0U,1U,0x7fffffffU,~0U}){auto changed=stop;changed[48]=value;assert(!L::StopShape(changed.data()));stopRefusals++;}
 for(unsigned request:{0x7ffffffeU,0x7fffffffU,~0U}){
  auto p=std::make_unique<P>();unsigned address=c->Base+c->Workspace(0)->offset+0x600000;
  assert(!p->Build(*c,0,0,0,0,0,1001,request,{1,2,3,4},source.data(),0,address) && !p->Outer());stopRefusals++;
 }
 printf("PASS QRB1 distinct STOP grammar/refusals%u/identity binding and completed-layer precondition remain ROOT obligations;NO native disposal grant\n",stopRefusals);
 printf("PASS QRB1 immutable complete recurrent HEAP DATA plans%u/mutations%u/refusals12/bytes%zu/all8canonical matrices+split cohort+norm+boot;NO producer/native/IO/commit grant\n",cases,mutations,sizeof(P));
}
