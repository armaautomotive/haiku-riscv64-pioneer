/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenMatrixPhaseDataBoundary.h"
#include "PioneerQ38TokenMatrixPhaseReadRole.h"
#ifdef PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE
#include "PioneerQ38TokenConstantQ4ProgramsRole.h"
static_assert(PioneerQ38TokenMatrixPhaseDataSelection==3
 && PioneerQ38TokenConstantQ4DataSelection==3,"actual separate public matrix-phase parent binding");
#elif defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT)
#include "PioneerQ38TokenConstantQ4ProgramsRole.h"
static_assert(PioneerQ38TokenMatrixPhaseDataSelection==2
 && PioneerQ38TokenConstantQ4DataSelection==2,"actual separate private matrix-phase parent binding");
#endif
