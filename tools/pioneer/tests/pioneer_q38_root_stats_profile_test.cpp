/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <initializer_list>
#define PIONEER_Q38_ROOT_STATS_BENCH
#include "../PioneerQ38RootStatsProfile.h"
int main() {
 uint32_t stats[64]={};stats[57]=0x31435951;
 for(unsigned i=0;i<3;i++){stats[58+2*i]=3+i;stats[59+2*i]=1+i;}
 for(unsigned i=0;i<6;i++)stats[6+i]=17+i;
 PioneerQ38RootStatsProfile p;
 for(unsigned sample=0;sample<1000;sample++)p.Add(stats);
 assert(p.valid && p.samples==1000);
 for(unsigned i=0;i<3;i++)assert(p.cycles[i]==1000*PioneerQ38RootCyclePhase(stats,i));
 for(unsigned i=0;i<6;i++)assert(p.packets[i]==1000*(17+i));
 p.Reset();assert(p.valid && !p.samples);for(auto v:p.cycles)assert(!v);for(auto v:p.packets)assert(!v);
 for(unsigned fault=0;fault<12;fault++) {
  auto bad=stats[57];p.Reset();
  if(fault==0){p.Add(nullptr);}
  else if(fault==1){stats[57]=0;p.Add(stats);stats[57]=bad;}
  else if(fault==2){p.samples=UINT64_MAX;p.Add(stats);assert(p.samples==UINT64_MAX);}
  else if(fault<6){p.cycles[fault-3]=UINT64_MAX;p.Add(stats);assert(p.cycles[fault-3]==UINT64_MAX);}
  else {p.packets[fault-6]=UINT64_MAX;p.Add(stats);assert(p.packets[fault-6]==UINT64_MAX);}
  assert(!p.valid);auto old=p;p.Add(stats);assert(p.samples==old.samples);
  for(unsigned i=0;i<3;i++)assert(p.cycles[i]==old.cycles[i]);
  for(unsigned i=0;i<6;i++)assert(p.packets[i]==old.packets[i]);
 }
 unsigned cases=0;
 for(uint64_t tag:{UINT64_C(0x2026102100010012),UINT64_C(0x2026102100010016)}) {
  tt_pioneer_driver_reload good={};good.version=1;good.driver=55;good.guard_version=1;
  good.build_tag=tag;good.generation=30;good.open_count=1;assert(PioneerQ38RootStatsDebugTag(good));
  for(unsigned bit=0;bit<64;bit++){auto r=good;r.build_tag^=UINT64_C(1)<<bit;
   bool known=r.build_tag==UINT64_C(0x2026102100010012) || r.build_tag==UINT64_C(0x2026102100010016);
   assert(PioneerQ38RootStatsDebugTag(r)==known);cases++;}
  for(unsigned field=0;field<7;field++)for(unsigned value:{0U,1U,2U,UINT32_MAX}) {
   auto r=good;uint32_t* targets[]={&r.version,&r.driver,&r.guard_version,&r.open_count,&r.quiesced,&r.cold,&r.active_owner};
   unsigned old=*targets[field];*targets[field]=value;assert(PioneerQ38RootStatsDebugTag(r)==(value==old));cases++;
  }
  auto r=good;r.generation=0;assert(!PioneerQ38RootStatsDebugTag(r));cases++;
 }
 printf("PASS 1000 exact64-bit host-copy profile samples/12 sticky invalid-overflow cases and %u exact-tag/idle checks;NO source/output/cleanup or card IO authority\n",cases);
}
