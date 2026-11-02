/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenMatrixPhaseDataBoundary.h"
#include "PioneerQ38TokenMatrixPhaseReadRole.h"
#ifdef PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT
#include "PioneerQ38TokenConstantQ4ProgramsRole.h"
static_assert(PioneerQ38TokenMatrixPhaseDataSelection==2
 && PioneerQ38TokenConstantQ4DataSelection==2,"actual separate private matrix-phase parent binding");
#endif
