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
 std::puts("PASS Work ioctl wall-time/paired-batch accounting and fail-closed overflow; NO DEVICE OR SPEED CLAIM");
}
