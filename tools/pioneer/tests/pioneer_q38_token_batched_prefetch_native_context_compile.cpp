/* SPDX-License-Identifier: MIT */
// Separate private renamed ABI only;no installable driver or Work grant.
#define PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS 1
#define PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT 1
#include "pioneer_q38_token_prefetch_native_context_compile.cpp"
static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==2
 && PioneerQ38TokenPrograms::BatchedPrefetch==1 && PioneerQ38TokenPrograms::Q8Batch==4
 && PioneerQ38TokenPrograms::Q4Batch==2 && PioneerQ38TokenPrograms::Q6Batch==2,
 "actual separate matching ROOT+worker private package binding");
