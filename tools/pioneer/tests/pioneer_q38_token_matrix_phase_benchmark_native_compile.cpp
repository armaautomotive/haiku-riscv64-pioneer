/* SPDX-License-Identifier: MIT */
// Distinct fixed34 sampler diagnostic,not Work or an installer. The inherited
// numeric tag alone is not provenance;freeze the whole public module identity.
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS 1
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE 1
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE 1
#include "pioneer_q38_token_constant_q4_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenMatrixPhaseDataSelection==3 && PioneerQ38TokenMatrixPhaseReadSelection==2
 && PioneerQ38TokenPrograms::MatrixPhase==1 && PioneerQ38TokenPrograms::MatrixPhaseVersion==2,
 "complete sampler and fresh-slot held reader in distinct bounded diagnostic subject");
