/* SPDX-License-Identifier: MIT */
// Separate fixed34 diagnostic, never Assistant Work or an installer.
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE 1
#include "pioneer_q38_wide_gdn_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenPhaseProfileDataSelection==3 && PioneerQ38TokenPrograms::TokenPhaseProfile==1,
 "actual complete timing ROOT in distinct bounded native subject");
