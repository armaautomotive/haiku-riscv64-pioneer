/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_WORK_RAW_UNPACK_CACHE
#define PIONEER_Q38_WORK_DIRECT_SRCB
#define PIONEER_Q38_STATIC_VECTOR
#include "../PioneerQ38VectorFixedFormat.h"
#include <cassert>
#include <vector>
#include <cstdio>
struct E {
 struct Event{unsigned kind,index;uint32_t value;};
 std::vector<Event> events;bool safe=false;
 void Op(uint32_t v){safe=false;events.push_back({0,0,v});}
 void Config(unsigned i,uint32_t v){assert(safe);events.push_back({1,i,v});}
 void Drain(){safe=true;events.push_back({2,0,0});}
};
int main(){
 unsigned panels=0;
 for(unsigned type:{8U,12U,14U})for(bool work:{false,true}) {
  unsigned sources[2]={~0U,~0U},loads=0,publishes=0;
  for(unsigned p=0;p<(type==8?2U:16U);p++) {
   E e;assert(PioneerQ38VectorFixedUnpack(e,type,p,work));
   unsigned raw=0,nop=0,gate=0,load0=0;bool upper=work&&type==12&&(p&1);
   for(unsigned i=0;i<e.events.size();i++){
    auto a=e.events[i];
    if(a.kind==0 && (a.value>>24)==0x42 && !(a.value&0x800000))raw++;
    if(a.kind==0 && a.value==0x43000007){
     assert(i && e.events[i-1].kind==0 && e.events[i-1].value==0xa2ff8020);
     assert(i+1<e.events.size() && e.events[i+1].kind==2);nop++;
    }
    if(a.kind==0 && a.value==0xa2ff8020)gate++;
    if(a.kind==0 && (a.value&0xfff00000U)==0x70000000U){
     unsigned reg=(a.value>>20)&15;if(reg==0){assert((a.value&1023)>=unsigned(upper?16:0) && (a.value&1023)<unsigned(upper?32:16));load0++;}
    }
   }
   assert(load0==8 && e.events.back().kind==2);
   bool reuse=work&&type==12&&(p&2);PioneerQ38VectorPanelSpan span;assert(PioneerQ38VectorPanel(type,p,span));
   if(reuse){assert(!raw && nop==1 && gate==1 && sources[p&1]==span.low);publishes++;}
   else {assert(raw==(type==14?2U:1U) && !nop && !gate);sources[p&1]=span.low;loads++;}
   panels++;
  }
  assert(loads==unsigned(work&&type==12?8:type==8?2:16));assert(publishes==unsigned(work&&type==12?8:0));
 }
 E bad;assert(!PioneerQ38VectorFixedUnpack(bad,12,16,true) && bad.events.empty());
 puts("PASS OFFLINE68 raw-cache panel plans:Q4 SAME sequential bytes reused only after TWO plane producers;all SrcA publications/C5 gates/final drains retained;Q8/Q6/checked unchanged;NO native authority or speed claim");assert(panels==68);
}
