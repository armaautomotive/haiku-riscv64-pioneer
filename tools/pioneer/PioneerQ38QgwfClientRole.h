/* SPDX-License-Identifier: MIT */
// Distinct QGWF tag143 diagnostic client; not an Assistant/Work mode.
#if defined(PIONEER_Q38_QGWF_CLIENT_143) && (PIONEER_Q38_QGWF_CLIENT_143 != 1 \
 || !defined(PIONEER_Q38_QGWD_CLIENT_141) || PIONEER_Q38_QGWD_CLIENT_141 != 1 \
 || !defined(PIONEER_Q38_QGWD_TWO_CLIENT_142) || PIONEER_Q38_QGWD_TWO_CLIENT_142 != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q38_QGWF_NATIVE_143) || defined(PIONEER_Q38_QGWD_NATIVE_141) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__))
#error QGWF client143 requires its exact non-Work two-token client profile
#endif
#ifndef PIONEER_Q38_QGWF_CLIENT_ROLE_BINDING_H
#define PIONEER_Q38_QGWF_CLIENT_ROLE_BINDING_H
#ifdef PIONEER_Q38_QGWF_CLIENT_143
enum {PioneerQ38QgwfClientRole=1};
#else
enum {PioneerQ38QgwfClientRole=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWF_CLIENT_143
static_assert(PioneerQ38QgwfClientRole==1,"late QGWF client selection");
#else
static_assert(PioneerQ38QgwfClientRole==0,"late QGWF client removal");
#endif
