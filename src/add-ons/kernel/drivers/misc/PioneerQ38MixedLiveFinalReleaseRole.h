/* SPDX-License-Identifier: MIT */
// No guard: this is an additional PRIVATE NODRV qualification selection,
// not permission to load a driver or reuse an old DEBUG/Work role.
#if !defined(PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE) || PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE != 1
#error Exact separate mixed final-release selection required,including defined0
#endif
#include "PioneerQ38MixedLiveNativeRole.h"
#include "PioneerQ38MixedLiveReadbackRole.h"
#ifdef PIONEER_Q38_PROJECTION_GRAPH_MEMORY
#error Old projection-memory aliases are not mixed final-release ownership,including defined0
#endif
