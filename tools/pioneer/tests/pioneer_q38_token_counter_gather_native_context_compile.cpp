/* SPDX-License-Identifier: MIT */
// Private renamed-ABI context ONLY,not installable or Work.
#define PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS 1
#define PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT 1
#include "pioneer_q38_token_tid_boundary_native_context_compile.cpp"
static_assert(PioneerQ38TokenCounterGatherSelection==2
 && PioneerQ38TokenPrograms::CounterGather==1
 && PioneerQ38TokenPrograms::TidBoundary==1,"actual new private firmware binding");
