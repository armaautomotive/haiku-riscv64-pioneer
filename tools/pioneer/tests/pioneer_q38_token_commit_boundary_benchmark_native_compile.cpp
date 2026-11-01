/* SPDX-License-Identifier: MIT */
// Explicit new physical diagnostic only;NO NODRV,CPU oracle or Work alias.
#define PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS 1
#define PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_retained_cohort_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenCommitBoundarySelection==3
 && PioneerQ38TokenCommitBoundaryPhysicalSelection==1
 && PioneerQ38TokenPrograms::CommitBoundarySeals==1,"distinct commit-boundary physical ROOT");
