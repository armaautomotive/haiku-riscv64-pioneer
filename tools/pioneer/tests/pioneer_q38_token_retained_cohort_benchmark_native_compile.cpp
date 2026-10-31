/* SPDX-License-Identifier: MIT */
// Explicit new physical diagnostic only;no NODRV,CPU oracle or Work alias.
#define PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS 1
#define PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_frontier_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenRetainedCohortSelection==3
 && PioneerQ38TokenRetainedCohortPhysicalSelection==1
 && PioneerQ38TokenPrograms::RetainedConvClosure==1,"distinct retained-cohort physical ROOT");
