/* SPDX-License-Identifier: MIT */
// Separate PRIVATE complete returning sampler,never an old code alias.
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS 1
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT 1
#include "pioneer_q38_token_constant_q4_native_context_compile.cpp"
static_assert(PioneerQ38TokenMatrixPhaseDataSelection==2
 && PioneerQ38TokenPrograms::MatrixPhase==1
 && PioneerQ38TokenPrograms::MatrixPhaseVersion==2
 && PioneerQ38TokenPrograms::MatrixPhaseAddress==0x84500
 && PioneerQ38TokenPrograms::MatrixPhaseWords==16,"actual complete sampler private code DATA");
