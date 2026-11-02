/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenMatrixPhaseDataBoundary.h"
#ifndef PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_SELECTION_H
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE
#ifdef PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE
enum {PioneerQ38TokenMatrixPhaseReadSelection=2};
#else
enum {PioneerQ38TokenMatrixPhaseReadSelection=1};
#endif
#else
enum {PioneerQ38TokenMatrixPhaseReadSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE
#if PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS) || PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS) || PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_IMAGE) || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_TEST) \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Exact separate sampler reader requires complete current code DATA,not shader,USER,oracle or Work
#endif
// Recheck the ENTIRE inherited production role on every read-role include,
// including late forbidden parent shader tags. No old helper can silently
// select this feature after the first namespace binding.
#include "PioneerQ38TokenConstantQ4ProgramsRole.h"
#ifdef PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE
#if PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_CONSTANT_Q4_BENCHMARK_NATIVE != 1 \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT)
#error Separate public sampler reader requires complete matching public parents
#endif
static_assert(PioneerQ38TokenMatrixPhaseReadSelection==2 && PioneerQ38TokenMatrixPhaseDataSelection==3,
 "late or mismatched public sampler readout binding");
#else
#if !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT != 1
#error Separate private sampler reader requires complete matching private parents
#endif
static_assert(PioneerQ38TokenMatrixPhaseReadSelection==1 && PioneerQ38TokenMatrixPhaseDataSelection==2,
 "late or mismatched private sampler readout binding");
#endif
#else
static_assert(PioneerQ38TokenMatrixPhaseReadSelection==0,"late sampler readout removal");
#endif
