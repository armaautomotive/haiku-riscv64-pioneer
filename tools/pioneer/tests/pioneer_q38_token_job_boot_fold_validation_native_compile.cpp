/* SPDX-License-Identifier: MIT */
// Conventional diagnostic with explicit new code revision;not Work or TEST.
#define PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS 1
#define PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE 1
#include "pioneer_q38_token_precise_beta_validation_native_compile.cpp"
static_assert(PioneerQ38TokenJobBootFoldSelection==3
 && PioneerQ38TokenJobBootFoldNativePhysicalSelection==1
 && PioneerQ38TokenPrograms::JobBootFold==1,"separate physical owner must contain optimized ROOT code");
