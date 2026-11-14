/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38WorkTiming.h"
#include <cassert>
#include <cstdio>

int main(){
 PioneerQ38WorkTiming p;
 assert(p.Add(5,502,100,250) && p.samples==1 && p.calls==497 && p.nanos==150);
 assert(p.Add(502,999,250,450) && p.samples==2 && p.calls==994 && p.nanos==350);
 assert(p.Add(999,1497,450,700,16,192) && p.samples==3 && p.calls==1492 && p.nanos==600 && p.pairedBatches==176);
 const auto saved=p;
 assert(!p.Add(2,1,1,2) && !p.Add(1,2,2,1) && !p.Add(1,1,1,2) && !p.Add(1,2,1,2,3,2));
 assert(p.calls==saved.calls && p.nanos==saved.nanos && p.pairedBatches==saved.pairedBatches && p.samples==saved.samples);
 p.calls=UINT64_MAX;assert(!p.Add(0,1,0,1));p.calls=0;
 p.nanos=UINT64_MAX;assert(!p.Add(0,1,0,1));p.nanos=0;
 p.pairedBatches=UINT64_MAX;assert(!p.Add(0,1,0,1,0,1));p.pairedBatches=0;
 p.samples=UINT32_MAX;assert(!p.Add(0,1,0,1));
 uint32_t stats[64]={};stats[57]=0x31435951;
 for(unsigned phase=0;phase<3;phase++)stats[58+2*phase]=10+phase;
 assert(PioneerQ38RootCycleDataValid(stats));
 uint64_t before[3]={},after[3]={},cycle[3]={10,11,12};
 p.ObserveRoot(0,1,before,cycle,true,true);
 assert(p.rootCycleDataValid&&p.rootSamples==1&&p.rootCycles[0]==10&&p.rootCycles[1]==11&&p.rootCycles[2]==12);
 uint64_t next[3]={12,15,20};p.ObserveRoot(1,2,cycle,next,true,true);
 assert(p.rootCycleDataValid&&p.rootSamples==2&&p.rootCycles[0]==12&&p.rootCycles[1]==15&&p.rootCycles[2]==20);
 PioneerQ38WorkTiming unavailable;unavailable.ObserveRoot(0,0,before,after,true,false);
 assert(!unavailable.rootCycleDataValid&&unavailable.calls==0&&unavailable.samples==0);
 PioneerQ38WorkTiming overflow;overflow.rootCycles[0]=UINT64_MAX;
 overflow.ObserveRoot(0,1,before,cycle,true,true);
 assert(!overflow.rootCycleDataValid&&overflow.rootCycles[0]==UINT64_MAX);
 std::puts("PASS Work ioctl wall-time/paired-batch accounting and fail-closed overflow; NO DEVICE OR SPEED CLAIM");
}
