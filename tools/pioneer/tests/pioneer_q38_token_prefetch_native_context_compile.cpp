/* SPDX-License-Identifier: MIT */
// Private renamed ABI only;not installable or Assistant Work.
#define PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT 1
#include "pioneer_q38_token_flat_matrix_native_context_compile.cpp"
static_assert(PioneerQ38TokenPrefetchDataSelection==2
 && PioneerQ38TokenPrograms::TilePrefetch==1 && PioneerQ38TokenPrograms::RawBuffers==2,
 "actual distinct private prefetch code binding");
