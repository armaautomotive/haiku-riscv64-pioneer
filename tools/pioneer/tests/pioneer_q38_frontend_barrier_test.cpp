/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_WORK_FRONTEND_BARRIER
#include "../PioneerQ38VectorFixedFormat.h"
#include <cassert>
#include <cstdio>
#include <vector>
struct Event {unsigned kind,index;uint32_t value;};
struct Program {
 std::vector<Event> ops;
 bool riscSafe=false;
 void Config(unsigned i,uint32_t v){assert(riscSafe);ops.push_back({1,i,v});}
 void Op(uint32_t v){riscSafe=false;ops.push_back({2,0,v});}
 void Drain(){riscSafe=true;ops.push_back({3,0,0});}
};
int main(){
 unsigned panels=0;
 for(unsigned type:{8U,12U,14U})for(unsigned panel=0;panel<(type==8?2U:16U);panel++) {
  Program checked,work;
  assert(PioneerQ38VectorFixedUnpack(checked,type,panel,false));
  assert(PioneerQ38VectorFixedUnpack(work,type,panel,true));
  unsigned cdrains=0,wdrains=0,gates=0;std::vector<Event> original,next;
  for(auto e:checked.ops){if(e.kind==3)cdrains++;else original.push_back(e);}
  for(auto e:work.ops) {
   if(e.kind==3){wdrains++;continue;}
   if(e.kind==2 && (e.value>>24)==0xa2) {
    assert(e.value==0xa2ff9816 && ((e.value>>15)&511)==511 && (e.value&8191)==0x1816);gates++;continue;
   }
   next.push_back(e);
  }
  assert(gates==2 && wdrains+2==cdrains && original.size()==next.size());
  for(unsigned i=0;i<original.size();i++)assert(original[i].kind==next[i].kind && original[i].index==next[i].index && original[i].value==next[i].value);
  // Full RISC-facing acknowledgements still bracket EACH MMIO config
  // update and final source/metadata consumer. No data address changes.
  assert(work.ops.back().kind==3 && work.ops.front().kind==2);panels++;
 }
 // A2 opcode/block[23:15]/condition[12:0],independent vendor diagram.
 assert(((0xa2101816U>>15)&511)==32 && ((0xa2101816U^0xa2ff9816U)&0xff001fffU)==0);
 puts("PASS OFFLINE34 exact panel sequences:ONLY TWO RISC acknowledgements replaced by full9-class SAME-condition frontend waits;all Tensix ops/MMIO addresses/config-facing/final drains unchanged;NO native or speed qualification");
 assert(panels==34);
}
