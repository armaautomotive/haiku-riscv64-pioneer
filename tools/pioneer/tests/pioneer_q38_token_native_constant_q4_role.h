/* SPDX-License-Identifier: MIT */
// Complete original first-layer/cancel/HOLD/readout/full-END TEST only.
#if !defined(PIONEER_Q38_TOKEN_NATIVE_CONSTANT_Q4_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CONSTANT_Q4_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS) || PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PHASE_PROFILE_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PHASE_PROFILE_FIXTURE != 1 \
 || defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_BENCHMARK_NATIVE) || defined(PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE) \
 || defined(PIONEER_Q38_Q4_CONSTANT_ISSUE) || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE)
#error Exact constant Q4 TEST requires complete original timing first-layer/cancel/HOLD/drain/full-END history
#endif
#include "../PioneerQ38TokenConstantQ4ProgramsRole.h"
#include "pioneer_q38_token_native_phase_profile_role.h"
static_assert(PioneerQ38TokenConstantQ4DataSelection==2
 && PioneerQ38TokenPrograms::ConstantQ4Issue==1 && PioneerQ38TokenPrograms::ConstantQ4WorkerRevision==1,
 "actual complete constant-Q4 worker ROM in this native instruction history");
