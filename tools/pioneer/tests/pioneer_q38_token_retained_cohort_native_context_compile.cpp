/* SPDX-License-Identifier: MIT */
// New private revision ONLY. ABI stays renamed;no public ioctl or install.
#define PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS 1
#define PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT 1
#include "pioneer_q38_token_frontier_native_context_compile.cpp"
static_assert(PioneerQ38TokenRetainedCohortSelection==2
 && PioneerQ38TokenPrograms::RetainedConvClosure==1,"actual retained-cohort ROOT/private owner binding");
