/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenManualTTSyncDataBoundary.h"
#ifdef PIONEER_Q38_TOKEN_MANUAL_TTSYNC_BENCHMARK_NATIVE
#include "PioneerQ38TokenConstantQ4ProgramsRole.h"
static_assert(PioneerQ38TokenManualTTSyncDataSelection==3 && PioneerQ38TokenConstantQ4DataSelection==3,
 "separate manual-completion public package,not an old DATA alias");
#elif defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_NATIVE_CONTEXT)
#include "PioneerQ38TokenConstantQ4ProgramsRole.h"
static_assert(PioneerQ38TokenManualTTSyncDataSelection==2 && PioneerQ38TokenConstantQ4DataSelection==2,
 "separate manual-completion private package,not an old DATA alias");
#endif
