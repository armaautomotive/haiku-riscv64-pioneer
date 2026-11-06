/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BF16_WORKER_PROBE_H
#define PIONEER_Q38_BF16_WORKER_PROBE_H
#include <stdint.h>
// HOST TEST shape bridge. The old generic ISA fixture uses its own Work
// TEST switches;the new BF16 DATA/arithmetic guards are not bypassed or
// redefined. Only this separately compiled observer includes the new ABI.
namespace PioneerQ38Bf16WorkerProbe {
enum : unsigned {Magic=0x34574751,Fault=0x46574754,Stop=2};
unsigned BootWord(unsigned);
bool BootShape(const uint32_t*);
bool JobShape(const uint32_t*,const uint32_t*,unsigned,unsigned);
}
#endif
