/* SPDX-License-Identifier: MIT */
// Distinct prepared-source backend. Old raw-Q4 clients/programs must fail
// closed rather than submitting their bytes to the new native UNPACK path.
#if !defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4) || PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4 != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) || PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_METRICS) || PIONEER_Q38_FULL_OFFLOAD_METRICS != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4) || defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_RESIDENT) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_PREPARED_Q4_ROOT_IMAGE) || defined(PIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PROGRAMS_H) || defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4_PROGRAMS_H) || defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4_PROGRAMS_H) \
 || defined(__FAST_MATH__)
#error Full-card prepared Q4 requires its distinct source/firmware role,not raw weights,CPU validation or hybrid Work
#endif
#if defined(PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL) && PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL != 1
#error Full-card prepared Q4 requires exact counter-unroll revision1
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE
#include "PioneerQ38FullOffloadPhaseProfileRole.h"
#endif
#ifdef _KERNEL_MODE
#if _KERNEL_MODE != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || PIONEER_Q38_FULL_OFFLOAD_NATIVE != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || TT_PIONEER_DRIVER_RELOAD != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) \
 || PIONEER_DRIVER_BUILD_TAG != (defined(PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY)?0x2026110500010100ULL:defined(PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE)?0x2026110500010099ULL:defined(PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL)?0x2026110500010098ULL:0x2026110500010097ULL) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT)
#error Full-card prepared Q4 requires its exact tag97 or counter-unroll tag98 native parent
#endif
#else
#if !defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || PIONEER_Q38_FULL_OFFLOAD_CLIENT != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_DRIVER_BUILD_TAG)
#error Full-card prepared Q4 requires its distinct USER client
#endif
#endif
