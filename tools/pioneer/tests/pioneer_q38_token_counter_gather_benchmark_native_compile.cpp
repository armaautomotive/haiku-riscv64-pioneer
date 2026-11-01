/* SPDX-License-Identifier: MIT */
// Bounded diagnostic only,NO CPU oracle or Assistant Work.
#define PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS 1
#define PIONEER_Q38_TOKEN_COUNTER_GATHER_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_tid_boundary_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenCounterGatherSelection==3
 && PioneerQ38TokenCounterGatherPhysicalSelection==1
 && PioneerQ38TokenPrograms::CounterGather==1,"actual new bounded diagnostic binding");
