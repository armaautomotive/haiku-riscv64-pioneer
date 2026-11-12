/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QgwfRateRole.h"
// Distinct QGWF two-token and frontier-rate diagnostic candidate identities. A build is not
// permission to install, execute, use Work, or skip separate validation.
#if defined(PIONEER_Q38_QGWF_NATIVE_143) && (PIONEER_Q38_QGWF_NATIVE_143 != 1 \
 || !defined(PIONEER_Q38_QGWD_NATIVE_141) || PIONEER_Q38_QGWD_NATIVE_141 != 1 \
 || !defined(PIONEER_Q38_QGWD_TWO_NATIVE_142) || PIONEER_Q38_QGWD_TWO_NATIVE_142 != 1 \
 || !defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || PIONEER_Q38_HYBRID_FULL_NATIVE != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || PIONEER_Q38_FULL_OFFLOAD_NATIVE != 1 \
 || !defined(PIONEER_Q38_QGWF_PROGRAMS_INSPECTION) || PIONEER_Q38_QGWF_PROGRAMS_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWF_DUAL_BATCH_PLAN) || PIONEER_Q38_QGWF_DUAL_BATCH_PLAN != 1 \
 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 || !defined(PIONEER_DRIVER_BUILD_TAG) \
 || (!defined(PIONEER_Q38_QGWF_RATE_NATIVE_144) && PIONEER_DRIVER_BUILD_TAG != 0x2026111100010143ULL) \
 || (defined(PIONEER_Q38_QGWF_RATE_NATIVE_144) && !defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145) && PIONEER_DRIVER_BUILD_TAG != 0x2026111100010144ULL) \
 || (defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145) && !defined(PIONEER_Q38_QGWD_RESIDENT_NATIVE_146) && PIONEER_DRIVER_BUILD_TAG != 0x2026111100010145ULL) \
 || (defined(PIONEER_Q38_QGWD_RESIDENT_NATIVE_146) && PIONEER_DRIVER_BUILD_TAG != 0x2026111100010146ULL) \
 || defined(PIONEER_Q38_QGWF_NATIVE_INSPECTION) || defined(PIONEER_Q38_QGWE_NATIVE_INSPECTION) \
 || defined(PIONEER_Q38_QGWF_FRONTIER_NATIVE_INSPECTION) \
 || (defined(PIONEER_Q38_QGWF_FRONTIER_PROGRAMS_INSPECTION) && !defined(PIONEER_Q38_QGWF_RATE_NATIVE_144)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__))
#error QGWF native143 requires its exact non-Work two-token candidate profile
#endif
#ifndef PIONEER_Q38_QGWF_NATIVE_CANDIDATE_BINDING_H
#define PIONEER_Q38_QGWF_NATIVE_CANDIDATE_BINDING_H
#ifdef PIONEER_Q38_QGWF_NATIVE_143
enum {PioneerQ38QgwfNativeCandidate=1};
#else
enum {PioneerQ38QgwfNativeCandidate=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWF_NATIVE_143
static_assert(PioneerQ38QgwfNativeCandidate==1,"late QGWF native candidate selection");
#else
static_assert(PioneerQ38QgwfNativeCandidate==0,"late QGWF native candidate removal");
#endif
