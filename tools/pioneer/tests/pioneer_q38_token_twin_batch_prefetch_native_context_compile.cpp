/* SPDX-License-Identifier: MIT */
// Private renamed ABI ONLY;never a public driver or Assistant Work.
#define PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS 1
#define PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT 1
#include "pioneer_q38_token_batched_prefetch_native_context_compile.cpp"
static_assert(PioneerQ38TokenTwinBatchPrefetchDataSelection==2
 && PioneerQ38TokenPrograms::TwinBatchPrefetch==1 && PioneerQ38TokenPrograms::FullBatchBuffers==2
 && PioneerQ38TokenPrograms::RawCopyBytes==0 && PioneerQ38TokenPrograms::SecondBatchAddress==0x9a000,
 "actual distinct complete copy-free twin-batch private code binding");
