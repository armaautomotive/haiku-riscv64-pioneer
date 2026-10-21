/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_MODEL_ROOT_PROFILE
#include "../PioneerQ38ModelRootProfile.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
static void Prefix(PioneerQ38ModelRootProfile& p) {
 for(unsigned pos=0;pos<5;pos++){p.Begin(0,pos);p.Add(8,5120,119,nullptr);p.End(true);}
 p.Begin(0,5);
}
int main(){
 uint32_t s[64]={};s[57]=0x31435951;
 for(unsigned i=0;i<3;i++){s[58+2*i]=3+i;s[59+2*i]=1+i;}
 for(unsigned i=0;i<6;i++)s[6+i]=17+i;
 PioneerQ38ModelRootProfile p;
 for(unsigned seq=0;seq<3;seq++)for(unsigned pos=0;pos<32;pos++) {
  assert(!p.PublishAllowed(true,true,false));p.Begin(seq,pos);
  for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})p.Add(type,k,119,pos<5?nullptr:s);
  p.End(true);
 }
 assert(p.PublishAllowed(true,true,false));
 for(const auto& b:p.bucket) {
  assert(b.samples==81 && b.workers==81*119);
  for(unsigned i=0;i<3;i++)assert(b.cycles[i]==81*PioneerQ38RootCyclePhase(s,i));
  for(unsigned i=0;i<6;i++)assert(b.packets[i]==81*(17+i));
 }
 assert(!p.PublishAllowed(false,true,false) && !p.PublishAllowed(true,false,false) && !p.PublishAllowed(true,true,true));
 unsigned refused=0;
 for(unsigned fault=0;fault<11;fault++) {
  PioneerQ38ModelRootProfile bad;Prefix(bad);auto& b=bad.bucket[0];
  if(!fault)b.samples=UINT64_MAX;else if(fault==1)b.workers=UINT64_MAX;
  else if(fault<5)b.cycles[fault-2]=UINT64_MAX;else b.packets[fault-5]=UINT64_MAX;
  auto old=b;bad.Add(8,5120,119,s);bad.Add(8,5120,119,s);bad.End(true);
  assert(!memcmp(&old,&b,sizeof(b)) && !bad.PublishAllowed(true,true,false));refused++;
 }
 for(unsigned fault=0;fault<14;fault++) {
  PioneerQ38ModelRootProfile bad;Prefix(bad);
  switch(fault) {
   case 0:bad.Add(0,5120,119,s);break;case 1:bad.Add(8,0,119,s);break;
   case 2:bad.Add(8,5120,0,s);break;case 3:bad.Add(8,5120,120,s);break;
   case 4:bad.Add(8,5120,119,nullptr);break;
   case 5:s[57]=0;bad.Add(8,5120,119,s);s[57]=0x31435951;break;
   case 6:bad.End(false);break;case 7:bad.End(true);break; // zero wave count
   case 8:bad.Begin(0,5);break;case 9:bad.Begin(1,0);break;
   case 10:bad.Begin(0,6);break;case 11:bad.Begin(3,0);break;
   case 12:bad.Begin(0,32);break;case 13:bad.Add(8,5120,119,s);bad.End(true);bad.End(true);break;
  }
  auto old=bad.bucket[0];bad.Add(8,5120,119,s);bad.Begin(0,0);bad.End(true);
  assert(!memcmp(&old,&bad.bucket[0],sizeof(old)) && !bad.PublishAllowed(true,true,false));refused++;
 }
 {PioneerQ38ModelRootProfile bad;bad.Add(8,5120,119,s);assert(!bad.PublishAllowed(true,true,false));refused++;}
 unsigned tags=0;
 for(uint64_t tag:{UINT64_C(0x2026102100010012),UINT64_C(0x2026102100010017),UINT64_C(0x2026102100010019),UINT64_C(0x2026102100010020)}) {
  tt_pioneer_driver_reload good={};good.version=1;good.driver=55;good.guard_version=1;good.generation=40;good.open_count=1;good.build_tag=tag;
  assert(PioneerQ38ModelRootProfileTag(good));
  for(unsigned bit=0;bit<64;bit++){auto r=good;r.build_tag^=UINT64_C(1)<<bit;
   bool known=r.build_tag==UINT64_C(0x2026102100010012) || r.build_tag==UINT64_C(0x2026102100010017) || r.build_tag==UINT64_C(0x2026102100010019) || r.build_tag==UINT64_C(0x2026102100010020);
   assert(PioneerQ38ModelRootProfileTag(r)==known);tags++;
  }
  for(unsigned field=0;field<8;field++) {
   auto r=good;switch(field){case 0:r.version=0;break;case 1:r.driver=0;break;case 2:r.guard_version=0;break;case 3:r.generation=0;break;
    case 4:r.open_count=0;break;case 5:r.active_owner=1;break;case 6:r.cold=1;break;case 7:r.quiesced=1;break;}
   assert(!PioneerQ38ModelRootProfileTag(r));tags++;
  }
 }
 printf("PASS model root CPU-only metrics:ALL9 shapes/96positions exactly81warm per shape,64bit sums,%u atomic-overflow/sticky/transition refusals,%u exact-tag checks;publication ONLY after clean complete THREE32;NO device/source/output/cleanup authority\n",refused,tags);
}
