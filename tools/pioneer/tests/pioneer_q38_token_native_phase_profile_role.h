/* SPDX-License-Identifier: MIT */
// Exact original first-layer/cancel/full-END TEST only; no clock timing grant.
#if !defined(PIONEER_Q38_TOKEN_NATIVE_PHASE_PROFILE_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PHASE_PROFILE_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS) || PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_WIDE_GDN_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_WIDE_GDN_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HOSTS_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HOSTS_FIXTURE != 1 \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT) || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE)
#error Distinct timing ROOT requires original wide-GDN first-layer/cancel/drain/full-END TEST;not USER,shader,public or full-model alias
#endif
#include "../PioneerQ38TokenPhaseProfileProgramsRole.h"
#include "pioneer_q38_token_native_wide_gdn_role.h"
static_assert(PioneerQ38TokenPhaseProfileDataSelection==2 && PioneerQ38TokenPrograms::TokenPhaseProfile==1
 && PioneerQ38TokenPrograms::PhaseProfileWords==60,"actual timing ROOT in this full native history");
