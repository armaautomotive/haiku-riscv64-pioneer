/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_bf16_worker_probe.h"
#include "../PioneerQ38Bf16WorkerLayout.h"
namespace PioneerQ38Bf16WorkerProbe {
namespace W=PioneerQ38Bf16WorkerLayout;
static_assert(unsigned(Magic)==unsigned(W::Magic) && unsigned(Fault)==unsigned(W::Fault),"exact observer data");
unsigned BootWord(unsigned i){return W::BootWord(i);}
bool BootShape(const uint32_t* b){return W::BootShape(b);}
bool JobShape(const uint32_t* j,const uint32_t* b,unsigned n,unsigned phase){return W::JobShape(j,b,n,phase);}
}
