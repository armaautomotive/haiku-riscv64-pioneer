/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QgwfNativeCandidateRole.h"
#include "PioneerQ38QgwfClientRole.h"
// Reserved identity for a future two-token diagnostic. This header grants no
// driver, device, Work-mode, or validation authority by itself.
#if defined(PIONEER_Q38_QGWD_TWO_NATIVE_142) && (PIONEER_Q38_QGWD_TWO_NATIVE_142 != 1 \
 || !defined(PIONEER_Q38_QGWD_NATIVE_141) || PIONEER_Q38_QGWD_NATIVE_141 != 1 \
 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) \
 || (!defined(PIONEER_Q38_QGWF_NATIVE_143) && PIONEER_DRIVER_BUILD_TAG != 0x2026111100010142ULL) \
 || (defined(PIONEER_Q38_QGWF_NATIVE_143) && !defined(PIONEER_Q38_QGWF_RATE_NATIVE_144) && PIONEER_DRIVER_BUILD_TAG != 0x2026111100010143ULL) \
 || (defined(PIONEER_Q38_QGWF_RATE_NATIVE_144) && !defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145) && PIONEER_DRIVER_BUILD_TAG != 0x2026111100010144ULL) \
 || (defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145) && PIONEER_DRIVER_BUILD_TAG != 0x2026111100010145ULL) \
 || defined(PIONEER_Q38_QGWD_TWO_CLIENT_142) || defined(PIONEER_Q38_QGWD_CLIENT_141) \
 || defined(PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__))
#error QGWD two-token native identity requires the isolated tag142 candidate role
#endif
#if defined(PIONEER_Q38_QGWD_TWO_CLIENT_142) && (PIONEER_Q38_QGWD_TWO_CLIENT_142 != 1 \
 || !defined(PIONEER_Q38_QGWD_CLIENT_141) || PIONEER_Q38_QGWD_CLIENT_141 != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_QGWD_NATIVE_141) \
 || defined(PIONEER_Q38_QGWD_TWO_NATIVE_142) || defined(PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__))
#error QGWD two-token client identity requires the isolated tag142 client role
#endif
#ifndef PIONEER_Q38_QGWD_TWO_TOKEN_IDENTITY_H
#define PIONEER_Q38_QGWD_TWO_TOKEN_IDENTITY_H
#if defined(PIONEER_Q38_QGWD_TWO_NATIVE_142)
enum {PioneerQ38QgwdTwoTokenIdentity=1421};
#elif defined(PIONEER_Q38_QGWD_TWO_CLIENT_142)
enum {PioneerQ38QgwdTwoTokenIdentity=1422};
#else
enum {PioneerQ38QgwdTwoTokenIdentity=0};
#endif
#endif
#if defined(PIONEER_Q38_QGWD_TWO_NATIVE_142)
static_assert(PioneerQ38QgwdTwoTokenIdentity==1421,"late two-token native identity change");
#elif defined(PIONEER_Q38_QGWD_TWO_CLIENT_142)
static_assert(PioneerQ38QgwdTwoTokenIdentity==1422,"late two-token client identity change");
#else
static_assert(PioneerQ38QgwdTwoTokenIdentity==0,"late two-token identity removal");
#endif
