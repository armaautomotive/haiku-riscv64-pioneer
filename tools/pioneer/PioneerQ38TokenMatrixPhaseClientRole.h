/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT_SELECTION_H
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT
enum {PioneerQ38TokenMatrixPhaseClientSelection=1};
#else
enum {PioneerQ38TokenMatrixPhaseClientSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT
#include "PioneerQ38TokenConstantQ4ClientRole.h"
#include "PioneerQ38TokenPhaseProfileClientRole.h"
#if PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT) || PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT) || PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT != 1
#error Distinct sampler USER client requires its complete constant-Q4/timing parents
#endif
#if defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_IMAGE) || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_PARENT_BINDING) \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_TEST) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Sampler USER client cannot alias firmware DATA,shader,native,CPU oracle or Work
#endif
static_assert(PioneerQ38TokenMatrixPhaseClientSelection==1,"late sampler USER selection");
#else
static_assert(PioneerQ38TokenMatrixPhaseClientSelection==0,"late sampler USER removal");
#endif
