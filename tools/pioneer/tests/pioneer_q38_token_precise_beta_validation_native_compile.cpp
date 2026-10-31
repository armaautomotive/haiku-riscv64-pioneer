/* SPDX-License-Identifier: MIT */
// Actual conventional diagnostic. No Work,CPU-reference or HOST test endpoint.
#define PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE 1
#include "pioneer_q38_posted_cancel_native_compile.cpp"
static_assert(PioneerQ38TokenPreciseBetaSelection==2
 && PioneerQ38TokenPreciseBetaNativePhysicalSelection==1
 && PioneerQ38TokenPrograms::PreciseBeta==1,"separate physical owner must contain precise-beta code");
