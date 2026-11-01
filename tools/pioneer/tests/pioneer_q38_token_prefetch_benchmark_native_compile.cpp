/* SPDX-License-Identifier: MIT */
// Bounded diagnostic only;NO runtime CPU model oracle or Assistant Work.
#define PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_flat_matrix_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenPrefetchDataSelection==3 && PioneerQ38TokenPrefetchPhysicalSelection==1
 && PioneerQ38TokenPrograms::TilePrefetch==1 && PioneerQ38TokenPrograms::RawBuffers==2,
 "actual distinct bounded prefetch code binding");
