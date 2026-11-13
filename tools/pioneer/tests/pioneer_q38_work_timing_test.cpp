/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38WorkTiming.h"
#include <cassert>
#include <cstdio>

int main(){
 PioneerQ38WorkTiming p;
 assert(p.Add(5,502,100,250) && p.samples==1 && p.calls==497 && p.nanos==150);
 assert(p.Add(502,999,250,450) && p.samples==2 && p.calls==994 && p.nanos==350);
 const auto saved=p;
 assert(!p.Add(2,1,1,2) && !p.Add(1,2,2,1) && !p.Add(1,1,1,2));
 assert(p.calls==saved.calls && p.nanos==saved.nanos && p.samples==saved.samples);
 p.calls=UINT64_MAX;assert(!p.Add(0,1,0,1));p.calls=0;
 p.nanos=UINT64_MAX;assert(!p.Add(0,1,0,1));p.nanos=0;
 p.samples=UINT32_MAX;assert(!p.Add(0,1,0,1));
 std::puts("PASS Work ioctl wall-time accounting and fail-closed overflow; NO DEVICE OR SPEED CLAIM");
}
