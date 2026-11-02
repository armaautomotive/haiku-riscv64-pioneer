/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38TokenPhaseProfileDataBoundary.h"
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS
#include "PioneerQ38TokenWideGdnRole.h"
static_assert(PioneerQ38TokenPhaseProfileParentBinding==1,"actual phase timing outer binding");
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE
static_assert(PioneerQ38TokenPhaseProfileDataSelection==3 && PioneerQ38TokenWideGdnDataSelection==3,
 "distinct public timing package with returning wide-GDN owner");
#elif defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT)
static_assert(PioneerQ38TokenPhaseProfileDataSelection==2 && PioneerQ38TokenWideGdnDataSelection==2,
 "distinct private timing package with returning wide-GDN owner");
#else
#error Pure phase timing DATA has no native readout or owner
#endif
#endif
