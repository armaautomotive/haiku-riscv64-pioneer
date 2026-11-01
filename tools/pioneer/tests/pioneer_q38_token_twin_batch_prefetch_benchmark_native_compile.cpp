/* SPDX-License-Identifier: MIT */
// Separately bound diagnostic only;NO runtime CPU model or Assistant Work.
#define PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS 1
#define PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_batched_prefetch_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenTwinBatchPrefetchDataSelection==3
 && PioneerQ38TokenTwinBatchPrefetchPhysicalSelection==1 && PioneerQ38TokenPrograms::TwinBatchPrefetch==1
 && PioneerQ38TokenPrograms::FullBatchBuffers==2 && PioneerQ38TokenPrograms::RawCopyBytes==0,
 "actual distinct bounded twin-batch diagnostic package binding");
