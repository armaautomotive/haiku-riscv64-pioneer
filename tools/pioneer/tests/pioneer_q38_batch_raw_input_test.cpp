/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_WORK_BATCH_RAW_INPUT
#define PIONEER_Q38_WORK_BATCH4_FETCH
#include "../PioneerQ38VectorFixedFormat.h"
#include <cassert>
#include <cstdio>
#include <vector>
struct Event {unsigned kind,index;uint32_t value;};
struct Program {
 std::vector<Event> ops;
 void Config(unsigned i,uint32_t v){ops.push_back({1,i,v});}
 void Op(uint32_t v){ops.push_back({2,0,v});}
 void Drain(){ops.push_back({3,0,0});}
};
int main(){
 unsigned rejected=0,panels=0;
 for(unsigned type:{8U,12U,14U}) {
  unsigned tile=type==8?576:type==12?2304:3392,batch=type==8?4:2;
  for(unsigned panel=0;panel<(type==8?2U:16U);panel++) {
   Program old;assert(PioneerQ38VectorFixedUnpack(old,type,panel,0x90000));
   for(unsigned t=0;t<batch;t++) {
    uint32_t base=0x98000+t*tile;Program next;
    assert(PioneerQ38VectorFixedUnpack(next,type,panel,base) && old.ops.size()==next.ops.size());
    unsigned pointers=0;
    for(unsigned i=0;i<old.ops.size();i++) {
     auto a=old.ops[i],b=next.ops[i];assert(a.kind==b.kind && a.index==b.index);
     if(a.kind==1 && a.index==76){assert(b.value==a.value+(base-0x90000)/16);pointers++;}
     else assert(a.value==b.value);
    }
    assert(pointers==(type==14?2U:1U));panels++;
   }
  }
  // Exhaustive aligned local-address negative controls use an independent
  // finite whitelist,not the selector's own interval/shape arithmetic.
  for(uint32_t base=0;base<0x200000;base+=4) {
   bool expected=base==0x90000 || base==0x98000 || base==0x98000+tile
    || (type==8 && (base==0x98480 || base==0x986c0));
   assert(PioneerQ38Batch4RawInput(type,base)==expected);
   if(expected)continue;
   Program bad;assert(!PioneerQ38VectorFixedUnpack(bad,type,0,base) && bad.ops.empty());rejected++;
  }
  for(uint32_t base:{0x98001U,0x9800fU,0x99a80U,UINT32_MAX}) {
   Program bad;assert(!PioneerQ38VectorFixedUnpack(bad,type,0,base) && bad.ops.empty());rejected++;
  }
 }
 for(unsigned type:{0U,1U,7U,9U,11U,13U,15U,UINT32_MAX})for(uint32_t base:{0x90000U,0x98000U}) {
  Program bad;assert(!PioneerQ38Batch4RawInput(type,base) && !PioneerQ38VectorFixedUnpack(bad,type,0,base) && bad.ops.empty());rejected++;
 }
 printf("PASS OFFLINE direct bounded-batch input:%u exact unchanged panel sequences,ONLY source base differs;%u invalid local/type selectors emit NOTHING;NO native/source/completion authority or speed claim\n",panels,rejected);
}
