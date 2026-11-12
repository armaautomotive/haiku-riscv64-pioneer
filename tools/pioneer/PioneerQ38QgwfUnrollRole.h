/* SPDX-License-Identifier: MIT */
// Separately tagged experimental worker image. No Work or reference role.
#if defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145) && (PIONEER_Q38_QGWF_UNROLL_NATIVE_145 != 1 \
 || !defined(PIONEER_Q38_QGWF_RATE_NATIVE_144) || PIONEER_Q38_QGWF_RATE_NATIVE_144 != 1 \
 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 || !defined(PIONEER_DRIVER_BUILD_TAG) \
 || PIONEER_DRIVER_BUILD_TAG != 0x2026111100010145ULL || defined(PIONEER_Q38_QGWF_UNROLL_CLIENT_145) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error Unrolled QGWF native145 requires its exact separate rate candidate
#endif
#if defined(PIONEER_Q38_QGWF_UNROLL_CLIENT_145) && (PIONEER_Q38_QGWF_UNROLL_CLIENT_145 != 1 \
 || !defined(PIONEER_Q38_QGWF_RATE_CLIENT_144) || PIONEER_Q38_QGWF_RATE_CLIENT_144 != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error Unrolled QGWF client145 requires its exact separate rate client
#endif
#ifndef PIONEER_Q38_QGWF_UNROLL_ROLE_BINDING_H
#define PIONEER_Q38_QGWF_UNROLL_ROLE_BINDING_H
#if defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145)
enum {PioneerQ38QgwfUnrollRole=1};
#elif defined(PIONEER_Q38_QGWF_UNROLL_CLIENT_145)
enum {PioneerQ38QgwfUnrollRole=2};
#else
enum {PioneerQ38QgwfUnrollRole=0};
#endif
#endif
#if defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145)
static_assert(PioneerQ38QgwfUnrollRole==1,"late unrolled QGWF native selection");
#elif defined(PIONEER_Q38_QGWF_UNROLL_CLIENT_145)
static_assert(PioneerQ38QgwfUnrollRole==2,"late unrolled QGWF client selection");
#else
static_assert(PioneerQ38QgwfUnrollRole==0,"late unrolled QGWF role removal");
#endif
