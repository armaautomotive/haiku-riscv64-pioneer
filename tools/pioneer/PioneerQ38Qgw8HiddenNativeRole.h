/* SPDX-License-Identifier: MIT */
// Strict, distinct tag129 hidden-state diagnostic. Never Work or CPU model
// validation. This selector may only extend the pinned QGW8 candidate family.
#ifdef PIONEER_Q38_QGW8_HIDDEN_NATIVE_129
#if PIONEER_Q38_QGW8_HIDDEN_NATIVE_129 != 1 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) || PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128 != 1 \
 || !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) || PIONEER_Q38_QGW8_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || PIONEER_Q38_HYBRID_FULL_NATIVE != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || PIONEER_Q38_FULL_OFFLOAD_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || TT_PIONEER_DRIVER_RELOAD != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x2026110700010129ULL \
 || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129) || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) \
 || defined(PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error QGW8 hidden DATA requires the isolated tag129 native diagnostic role
#endif
#endif
#ifndef PIONEER_Q38_QGW8_HIDDEN_NATIVE_BINDING_H
#define PIONEER_Q38_QGW8_HIDDEN_NATIVE_BINDING_H
#ifdef PIONEER_Q38_QGW8_HIDDEN_NATIVE_129
enum {PioneerQ38Qgw8HiddenNativeSelection=1};
#else
enum {PioneerQ38Qgw8HiddenNativeSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_HIDDEN_NATIVE_129
static_assert(PioneerQ38Qgw8HiddenNativeSelection==1,"late QGW8 hidden native selection");
#else
static_assert(PioneerQ38Qgw8HiddenNativeSelection==0,"late QGW8 hidden native removal");
#endif
