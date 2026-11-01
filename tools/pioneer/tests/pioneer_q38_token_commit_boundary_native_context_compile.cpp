/* SPDX-License-Identifier: MIT */
// New private revision ONLY. Conventional ABI stays renamed;NO install.
#define PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS 1
#define PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT 1
#include "pioneer_q38_token_retained_cohort_native_context_compile.cpp"
static_assert(PioneerQ38TokenCommitBoundarySelection==2
 && PioneerQ38TokenPrograms::CommitBoundarySeals==1
 && PioneerQ38TokenPrograms::RetainedConvClosure==1,"actual new firmware/private owner binding");
