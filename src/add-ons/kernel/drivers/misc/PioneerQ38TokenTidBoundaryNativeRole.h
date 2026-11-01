/* SPDX-License-Identifier: MIT */
// Private renamed-ABI NODRV context only;NOT installable or Assistant Work.
#if !defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS) || PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS) || PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS != 1
#error Exact TID-boundary private context and packet-identity parents required
#endif
#if defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TID_BOUNDARY_IMAGE)
#error Packet-identity private context cannot alias diagnostic or ROOT BUILD
#endif
#include "PioneerQ38TokenPacketIdentityBoundaryNativeRole.h"
