/* SPDX-License-Identifier: MIT */
// Private non-loadable inspection only; no control operation or device grant.
#ifdef PIONEER_Q38_QGW8_TRACE_READ_INSPECTION
#if PIONEER_Q38_QGW8_TRACE_READ_INSPECTION != 1 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) || PIONEER_Q38_QGW8_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) || PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x51544b4e4f445256ULL \
 || defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) \
 || defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) \
 || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129) \
 || defined(PIONEER_Q38_QGW8_TRACE_CLIENT_131) \
 || defined(PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE) || defined(PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error QGW8 trace read requires the isolated non-loadable inspection role
#endif
#include "PioneerQ38Qgw8NativeInspectionRole.h"
#endif
#ifndef PIONEER_Q38_QGW8_TRACE_READ_BINDING_H
#define PIONEER_Q38_QGW8_TRACE_READ_BINDING_H
#ifdef PIONEER_Q38_QGW8_TRACE_READ_INSPECTION
enum {PioneerQ38Qgw8TraceReadInspection=1};
#else
enum {PioneerQ38Qgw8TraceReadInspection=0};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_TRACE_READ_INSPECTION
static_assert(PioneerQ38Qgw8TraceReadInspection==1,"late trace read selection");
#else
static_assert(PioneerQ38Qgw8TraceReadInspection==0,"late trace read removal");
#endif
