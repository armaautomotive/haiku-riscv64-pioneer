/* SPDX-License-Identifier: MIT */
// Bounded diagnostic only;no runtime CPU model oracle or Assistant Work.
#define PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS 1
#define PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_prefetch_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==3
 && PioneerQ38TokenBatchedPrefetchPhysicalSelection==1
 && PioneerQ38TokenPrograms::BatchedPrefetch==1 && PioneerQ38TokenPrograms::Q8Batch==4
 && PioneerQ38TokenPrograms::Q4Batch==2 && PioneerQ38TokenPrograms::Q6Batch==2,
 "actual distinct bounded batched code binding");
