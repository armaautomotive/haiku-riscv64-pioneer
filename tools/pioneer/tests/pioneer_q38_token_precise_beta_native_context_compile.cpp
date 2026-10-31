/* SPDX-License-Identifier: MIT */
// Actual private native owner, no conventional loadable driver ABI.
#define PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS 1
#include "pioneer_q38_token_fault_native_context_compile.cpp"
static_assert(PioneerQ38TokenPreciseBetaSelection==1,"separate immutable beta math selection");
static_assert(PioneerQ38TokenPrograms::PreciseBeta==1,"actual native owner must name the new code");
