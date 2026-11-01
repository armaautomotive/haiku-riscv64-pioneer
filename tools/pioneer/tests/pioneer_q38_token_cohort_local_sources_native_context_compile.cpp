/* SPDX-License-Identifier: MIT */
// Private renamed-ABI context only;NOT an installable conventional driver.
#define PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS 1
#define PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_NATIVE_CONTEXT 1
#include "pioneer_q38_token_commit_boundary_native_context_compile.cpp"
static_assert(PioneerQ38TokenCohortLocalSourcesSelection==2
 && PioneerQ38TokenPrograms::CohortLocalSources==1 && PioneerQ38TokenPrograms::ColdSourceFromStatus==1,
 "actual new private firmware binding");
