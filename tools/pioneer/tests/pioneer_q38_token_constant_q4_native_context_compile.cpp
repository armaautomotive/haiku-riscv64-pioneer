/* SPDX-License-Identifier: MIT */
// Separate PRIVATE constant-Q4 full worker + unchanged full timing ROOT.
#define PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS 1
#define PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT 1
#include "pioneer_q38_token_phase_profile_native_context_compile.cpp"
static_assert(PioneerQ38TokenConstantQ4DataSelection==2
 && PioneerQ38TokenPrograms::ConstantQ4Issue==1
 && PioneerQ38TokenPrograms::ConstantQ4WorkerRevision==1,
 "actual distinct constant Q4 full worker in private native context");
