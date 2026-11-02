/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS) || PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONSTANT_Q4_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CONSTANT_Q4_FIXTURE != 1 \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_TEST) || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE)
#error Exact sampler TEST requires original complete constant-Q4 first-layer/cancel/HOLD/drain/full-END history
#endif
#include "../PioneerQ38TokenMatrixPhaseProgramsRole.h"
#include "pioneer_q38_token_native_constant_q4_role.h"
static_assert(PioneerQ38TokenMatrixPhaseDataSelection==2
 && PioneerQ38TokenPrograms::MatrixPhase==1 && PioneerQ38TokenPrograms::MatrixPhaseVersion==2,
 "actual frozen complete sampler code in this original native history");
