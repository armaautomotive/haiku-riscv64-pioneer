/* SPDX-License-Identifier: MIT */
// Private renamed-ABI NODRV only;not installable or Assistant Work.
#if !defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS) || PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS) || PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS != 1
#error Exact counter-gather private context and TID parents required
#endif
#if defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_BENCHMARK_NATIVE) || defined(PIONEER_Q38_COUNTER_GATHER_IMAGE)
#error Counter-gather private context cannot alias diagnostic or ROOT BUILD
#endif
#include "PioneerQ38TokenTidBoundaryNativeRole.h"
