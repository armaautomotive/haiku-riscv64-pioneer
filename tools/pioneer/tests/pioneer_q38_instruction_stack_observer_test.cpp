/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_INSTRUCTION_STACK_OBSERVER_TEST 1
#include "PioneerQ38InstructionStackObserver.h"
#include "pioneer_q38_native_peer_test_bridge.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
static_assert(sizeof(Q38NativePeerTestStats)==64 && sizeof(Q38NativePeerTestStack)==16,"opaque existing ABI preserved");
int main(){
 Q38InstructionStackObserver s;assert(s.Valid() && !s.Initialized() && !s.Instructions() && s.MinimumSp()==0x70000);
 assert(s.Retired(0) && s.Retired(0) && !s.Initialized());
 unsigned count=2;
 for(unsigned sp=0x70000;;sp-=4){assert(s.Retired(sp));count++;if(sp==0x6e000)break;}
 for(unsigned sp=0x6e000;sp<=0x70000;sp+=4){assert(s.Retired(sp));count++;}
 assert(s.Valid() && s.Initialized() && s.Instructions()==count && count==4100 && s.MinimumSp()==0x6e000);
 assert(!s.Retired(0) && !s.Valid());const auto n=s.Instructions();
 assert(!s.Retired(0x70000) && s.Instructions()==n && s.MinimumSp()==0x6e000);
 unsigned refused=0;
 for(unsigned sp:{1U,2U,3U,0x6dffcU,0x6e001U,0x6ffffU,0x70001U,0x70004U,0xfffffffcU,UINT32_MAX}){
  Q38InstructionStackObserver bad;assert(!bad.Retired(sp) && !bad.Valid() && !bad.Initialized());
  assert(!bad.Retired(0x70000) && bad.Instructions()==1);refused++;
 }
 puts("PASS HOST stack observer/all2049 aligned SP values both directions/boot-zero then zero-after-init refusal/sticky invalid+range+alignment failures/old64-byte Stats preserved;no stack memory access,processor mutation,native/physical/whole-model/combined-stack or speed proof");
 assert(refused==10);
}
