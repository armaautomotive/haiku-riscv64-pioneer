/* SPDX-License-Identifier: MIT */
// Standalone CPU-metadata compile/frame check,NO native role or card IO.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HostDeadline.h"
static_assert(sizeof(PioneerQ38HostDeadline)<=32,"small CPU-only interval record");
extern "C" bool pioneer_host_deadline_begin(PioneerQ38HostDeadline& p,uint64_t now){return p.Begin(now);}
extern "C" bool pioneer_host_deadline_observe(PioneerQ38HostDeadline& p,uint64_t now){return p.Observe(now);}
extern "C" bool pioneer_host_deadline_composed(uint64_t begin,uint64_t middle,uint64_t end){
 PioneerQ38HostDeadline p;return p.Begin(begin) && p.Observe(middle) && p.Observe(end);
}
