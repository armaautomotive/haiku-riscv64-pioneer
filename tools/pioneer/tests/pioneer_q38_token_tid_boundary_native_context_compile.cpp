/* SPDX-License-Identifier: MIT */
// Private renamed-ABI context ONLY;not an installable conventional driver.
#define PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS 1
#define PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT 1
#include "pioneer_q38_token_packet_identity_boundary_native_context_compile.cpp"
static_assert(PioneerQ38TokenTidBoundarySelection==2
 && PioneerQ38TokenPrograms::TidBoundary==1
 && PioneerQ38TokenPrograms::PacketIdentityBoundary==1,"actual new private firmware binding");
