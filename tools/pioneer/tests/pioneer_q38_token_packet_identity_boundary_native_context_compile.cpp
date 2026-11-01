/* SPDX-License-Identifier: MIT */
// Private renamed-ABI context ONLY;not an installable conventional driver.
#define PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_NATIVE_CONTEXT 1
#include "pioneer_q38_token_cohort_local_sources_native_context_compile.cpp"
static_assert(PioneerQ38TokenPacketIdentityBoundarySelection==2
 && PioneerQ38TokenPrograms::PacketIdentityBoundary==1
 && PioneerQ38TokenPrograms::CohortLocalSources==1,"actual new private firmware binding");
