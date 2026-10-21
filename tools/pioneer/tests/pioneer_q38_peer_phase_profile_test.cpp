/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_WORK_PEER_PHASE_PROFILE
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "../PioneerQ38PeerPhaseProfile.h"
static void Invalid(const PioneerQ38PeerPhaseProfile& p){uint32_t out[16];for(auto& w:out)w=UINT32_MAX;assert(!p.Describe(out,true)&&!out[7]);for(unsigned w=8;w<16;w++)assert(!out[w]);}
int main(){
 unsigned cases=0;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U}){
  PioneerQ38PeerPhaseProfile p(119,31,type,k,groups);unsigned blocks=k>>(type==8?5:8),panels=type==8?2:16;
  uint64_t t=UINT64_C(1)<<40,fetch=0,decode=0,execute=0;
  for(unsigned group=0;group<groups;group++)for(unsigned block=0;block<blocks;block++){
   Invalid(p);assert(p.BeginBlock(group,block,t));t+=13;fetch+=13;assert(p.Fetched(t));
   for(unsigned panel=0;panel<panels;panel++){
    t+=17;decode+=17;assert(p.Decoded(panel,t));t+=23;execute+=23;assert(p.Executed(panel,t));
   }
   t+=101; // unmeasured job/group/export gap MUST NOT enter a hot phase
  }
  uint32_t out[16];assert(p.Complete()&&!p.Describe(out,false)&&p.Describe(out,true));
  assert(out[0]==0x31545051&&out[1]==1&&out[2]==31&&out[3]==119&&out[4]==type&&out[5]==k&&out[6]==groups&&out[7]==1&&!out[14]&&!out[15]);
  assert(((uint64_t(out[9])<<32)|out[8])==fetch&&((uint64_t(out[11])<<32)|out[10])==decode&&((uint64_t(out[13])<<32)|out[12])==execute);cases++;
 }
 for(unsigned fault=0;fault<14;fault++){
  PioneerQ38PeerPhaseProfile p(1,1,12,5120,1);uint32_t out[16];
  if(fault==0){assert(!p.Fetched(1));}
  else if(fault==1){assert(!p.BeginBlock(0,1,100));}
  else if(fault==2){assert(!p.BeginBlock(1,0,100));}
  else if(fault==3){assert(!p.BeginBlock(0,0,UINT64_MAX));}
  else{
   assert(p.BeginBlock(0,0,100));
   if(fault==4)assert(!p.BeginBlock(0,0,101));
   if(fault==5)assert(!p.Decoded(0,101));
   if(fault==6)assert(!p.Fetched(99));
   if(fault==7)assert(!p.Fetched(UINT64_MAX));
   if(fault>=8){assert(p.Fetched(101));
    if(fault==8)assert(!p.Fetched(102));
    if(fault==9)assert(!p.Decoded(1,102));
    if(fault==10)assert(!p.Executed(0,102));
    if(fault==11){assert(p.Decoded(0,102));assert(!p.Executed(1,103));}
    if(fault==12)p.BadClock();
    if(fault==13){uint64_t t=102;for(unsigned panel=0;panel<16;panel++){assert(p.Decoded(panel,t++));assert(p.Executed(panel,t++));}assert(!p.BeginBlock(0,1,100));}
   }
  }
  Invalid(p);assert(!p.BeginBlock(0,0,100)&&!p.Fetched(101)&&!p.Decoded(0,102)&&!p.Executed(0,103)&&!p.Describe(out,true));
 }
 for(unsigned w:{0U,120U}){PioneerQ38PeerPhaseProfile p(w,1,12,5120,1);Invalid(p);}
 for(unsigned n:{0U,0x7fffffffU}){PioneerQ38PeerPhaseProfile p(1,n,12,5120,1);Invalid(p);}
 for(unsigned type:{0U,7U,15U}){PioneerQ38PeerPhaseProfile p(1,1,type,5120,1);Invalid(p);}
 for(unsigned k:{0U,5119U,17409U}){PioneerQ38PeerPhaseProfile p(1,1,12,k,1);Invalid(p);}
 for(unsigned g:{0U,17U}){PioneerQ38PeerPhaseProfile p(1,1,12,5120,g);Invalid(p);}
 printf("PASS %u bounded type/K/group exact hot-phase schedules;unmeasured gaps excluded,identity/full invalid envelope,14 sticky clock/order/phase faults+12 shape refusals;DATA ONLY NO IO/source/ownership/cleanup/native-speed authority\n",cases);
}
