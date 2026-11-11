/* SPDX-License-Identifier: MIT */
// Non-loadable QGWE compile inspection, layered on the renamed fake-tag
// QGWD inspection. It grants no device, install, client or Work authority.
#ifdef PIONEER_Q38_QGWE_NATIVE_INSPECTION
#if PIONEER_Q38_QGWE_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION) || PIONEER_Q38_QGWD_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) || PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_QGWE_PROGRAMS_INSPECTION) || PIONEER_Q38_QGWE_PROGRAMS_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWE_BATCH_DENSE_PLAN) || PIONEER_Q38_QGWE_BATCH_DENSE_PLAN != 1 \
 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 || !defined(PIONEER_DRIVER_BUILD_TAG) \
 || PIONEER_DRIVER_BUILD_TAG != 0x51544b4e4f445256ULL \
 || !defined(api_version) || !defined(init_hardware) || !defined(init_driver) || !defined(uninit_driver) \
 || !defined(publish_devices) || !defined(find_device) || defined(PIONEER_Q38_QGWD_NATIVE_141) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGWE native inspection requires the renamed fake-tag compile-only role
#endif
#endif
#ifndef PIONEER_Q38_QGWE_NATIVE_INSPECTION_BINDING_H
#define PIONEER_Q38_QGWE_NATIVE_INSPECTION_BINDING_H
#ifdef PIONEER_Q38_QGWE_NATIVE_INSPECTION
enum {PioneerQ38QgweNativeInspection=1};
#else
enum {PioneerQ38QgweNativeInspection=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWE_NATIVE_INSPECTION
static_assert(PioneerQ38QgweNativeInspection==1,"late QGWE native inspection selection");
#else
static_assert(PioneerQ38QgweNativeInspection==0,"late QGWE native inspection removal");
#endif
