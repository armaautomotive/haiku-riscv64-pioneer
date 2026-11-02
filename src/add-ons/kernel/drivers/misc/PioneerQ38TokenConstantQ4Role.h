/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38TokenConstantQ4DataBoundary.h"
#ifdef PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS
#include "PioneerQ38TokenPhaseProfileRole.h"
static_assert(PioneerQ38TokenConstantQ4ParentBinding==1,"actual constant Q4 outer binding");
#ifdef PIONEER_Q38_TOKEN_CONSTANT_Q4_BENCHMARK_NATIVE
static_assert(PioneerQ38TokenConstantQ4DataSelection==3 && PioneerQ38TokenPhaseProfileDataSelection==3,
 "distinct public constant Q4 package and complete timing owner");
#elif defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT)
static_assert(PioneerQ38TokenConstantQ4DataSelection==2 && PioneerQ38TokenPhaseProfileDataSelection==2,
 "distinct private constant Q4 package and complete timing owner");
#else
#error Pure constant Q4 program DATA has no native owner
#endif
#endif
