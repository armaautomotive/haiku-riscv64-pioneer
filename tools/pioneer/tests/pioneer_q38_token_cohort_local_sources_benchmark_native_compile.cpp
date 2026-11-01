/* SPDX-License-Identifier: MIT */
// Bounded physical diagnostic only;NO CPU oracle or Assistant Work route.
#define PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS 1
#define PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_commit_boundary_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenCohortLocalSourcesSelection==3
 && PioneerQ38TokenCohortLocalSourcesPhysicalSelection==1
 && PioneerQ38TokenPrograms::CohortLocalSources==1,"actual new bounded diagnostic binding");
