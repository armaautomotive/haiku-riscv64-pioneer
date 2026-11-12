/* SPDX-License-Identifier: MIT */
// Bounded timing diagnostic only. Never an Assistant/Work or CPU-reference role.
#include "PioneerQ38QgwfUnrollRole.h"
#if defined(PIONEER_Q38_QGWF_RATE_NATIVE_144) && (PIONEER_Q38_QGWF_RATE_NATIVE_144 != 1 \
 || !defined(PIONEER_Q38_QGWF_NATIVE_143) || PIONEER_Q38_QGWF_NATIVE_143 != 1 \
 || !defined(PIONEER_Q38_QGWF_FRONTIER_PROGRAMS_INSPECTION) || PIONEER_Q38_QGWF_FRONTIER_PROGRAMS_INSPECTION != 1 \
 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 || !defined(PIONEER_DRIVER_BUILD_TAG) \
 || (!defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145) && PIONEER_DRIVER_BUILD_TAG != 0x2026111100010144ULL) \
 || (defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145) && PIONEER_DRIVER_BUILD_TAG != 0x2026111100010145ULL) \
 || defined(PIONEER_Q38_QGWF_RATE_CLIENT_144) \
 || defined(PIONEER_Q38_QGWF_FRONTIER_NATIVE_INSPECTION) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGWF rate144 native requires its exact non-Work frontier candidate
#endif
#if defined(PIONEER_Q38_QGWF_RATE_CLIENT_144) && (PIONEER_Q38_QGWF_RATE_CLIENT_144 != 1 \
 || !defined(PIONEER_Q38_QGWF_CLIENT_143) || PIONEER_Q38_QGWF_CLIENT_143 != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_QGWF_RATE_NATIVE_144) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGWF rate144 client requires its exact non-Work diagnostic profile
#endif
#ifndef PIONEER_Q38_QGWF_RATE_ROLE_BINDING_H
#define PIONEER_Q38_QGWF_RATE_ROLE_BINDING_H
#if defined(PIONEER_Q38_QGWF_RATE_NATIVE_144)
enum {PioneerQ38QgwfRateRole=1};
#elif defined(PIONEER_Q38_QGWF_RATE_CLIENT_144)
enum {PioneerQ38QgwfRateRole=2};
#else
enum {PioneerQ38QgwfRateRole=0};
#endif
#endif
#if defined(PIONEER_Q38_QGWF_RATE_NATIVE_144)
static_assert(PioneerQ38QgwfRateRole==1,"late QGWF rate native selection");
#elif defined(PIONEER_Q38_QGWF_RATE_CLIENT_144)
static_assert(PioneerQ38QgwfRateRole==2,"late QGWF rate client selection");
#else
static_assert(PioneerQ38QgwfRateRole==0,"late QGWF rate role removal");
#endif
