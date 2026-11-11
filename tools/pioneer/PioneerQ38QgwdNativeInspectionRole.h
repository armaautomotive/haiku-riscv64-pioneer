/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QgwdNativeCandidateRole.h"
#include "PioneerQ38QgweNativeInspectionRole.h"
// Rechecked on every include. The ordinary inspection profile uses renamed
// entry points and a fake tag. Only the separately checked tag141 role may
// select this geometry in a candidate; neither profile is hardware approval.
#if defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION) && !defined(PIONEER_Q38_QGWD_NATIVE_141)
#if PIONEER_Q38_QGWD_NATIVE_INSPECTION != 1 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) || PIONEER_Q38_QGW8_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) || PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD) || PIONEER_Q38_HYBRID_TRACKED_UPLOAD != 1 \
 || !defined(PIONEER_Q38_QGWD_PROGRAMS_INSPECTION) || PIONEER_Q38_QGWD_PROGRAMS_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWD_PLAN_SIMULATION) || PIONEER_Q38_QGWD_PLAN_SIMULATION != 1 \
 || !defined(PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION) || PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWD_CONTROL_INSPECTION) || PIONEER_Q38_QGWD_CONTROL_INSPECTION != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || TT_PIONEER_DRIVER_RELOAD != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x51544b4e4f445256ULL \
 || !defined(api_version) || !defined(init_hardware) || !defined(init_driver) || !defined(uninit_driver) \
 || !defined(publish_devices) || !defined(find_device) || defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) \
 || defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) || defined(PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY) \
 || defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error QGWD native inspection requires exclusive renamed fake-tag non-loadable profile
#endif
#endif
#ifndef PIONEER_Q38_QGWD_NATIVE_INSPECTION_BINDING_H
#define PIONEER_Q38_QGWD_NATIVE_INSPECTION_BINDING_H
#ifdef PIONEER_Q38_QGWD_NATIVE_INSPECTION
enum {PioneerQ38QgwdNativeInspectionRole=1};
#else
enum {PioneerQ38QgwdNativeInspectionRole=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWD_NATIVE_INSPECTION
static_assert(PioneerQ38QgwdNativeInspectionRole==1,"late QGWD native inspection selection");
#else
static_assert(PioneerQ38QgwdNativeInspectionRole==0,"late QGWD native inspection removal");
#endif
