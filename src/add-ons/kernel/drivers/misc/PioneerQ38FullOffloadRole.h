/* SPDX-License-Identifier: MIT */
// Rechecked on every include. A new explicit role, never an old diagnostic
// promoted by changing its request shape or claiming its success as Work.
#if !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || PIONEER_Q38_FULL_OFFLOAD_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_EMIT_CONTEXT_BODY) || PIONEER_Q38_TOKEN_EMIT_CONTEXT_BODY != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || PIONEER_Q38_TOKEN_PUSHED_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_DECODER_RUNTIME) || PIONEER_Q38_DECODER_RUNTIME != 1 \
 || !defined(PIONEER_Q38_GDN_MUTABLE_STATE) || PIONEER_Q38_GDN_MUTABLE_STATE != 1 \
 || !defined(PIONEER_Q38_FFN_NATIVE_GRAPH) || PIONEER_Q38_FFN_NATIVE_GRAPH != 1 \
 || !defined(PIONEER_Q38_FFN_PERSISTENT_GRAPH) || PIONEER_Q38_FFN_PERSISTENT_GRAPH != 1 \
 || !defined(PIONEER_Q38_FFN_GRAPH_INPUT_ROOT) || PIONEER_Q38_FFN_GRAPH_INPUT_ROOT != 1 \
 || !defined(PIONEER_Q38_FFN_MODEL_INIT) || PIONEER_Q38_FFN_MODEL_INIT != 1 \
 || !defined(PIONEER_Q38_NORMAL_CLOCK) || PIONEER_Q38_NORMAL_CLOCK != 1 \
 || !defined(PIONEER_Q38_SPAN_PRELOAD) || PIONEER_Q38_SPAN_PRELOAD != 1 \
 || !defined(PIONEER_Q38_SPAN_FAST) || PIONEER_Q38_SPAN_FAST != 1 \
 || !defined(PIONEER_Q38_LOCAL_DELTA_PROBE) || PIONEER_Q38_LOCAL_DELTA_PROBE != 1 \
 || !defined(TT_PIONEER_EXPERIMENTAL_Q38_BANKS) || TT_PIONEER_EXPERIMENTAL_Q38_BANKS != 1 \
 || !defined(TT_PIONEER_EXPERIMENTAL_Q38_PANEL) || TT_PIONEER_EXPERIMENTAL_Q38_PANEL != 1 \
 || !defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT) || TT_PIONEER_EXPERIMENTAL_Q38_COHORT != 1 \
 || !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH) || TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH != 1 \
 || !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR) || TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR != 1 \
 || !defined(TT_PIONEER_EXPERIMENTAL_Q38_STREAM) || TT_PIONEER_EXPERIMENTAL_Q38_STREAM != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || TT_PIONEER_DRIVER_RELOAD != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG)
#error Full offload requires the exact native role and all resident parents
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
#include "PioneerQ38FullOffloadPreparedQ4Role.h"
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4
#include "PioneerQ38FullOffloadMacroQ4Role.h"
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4
#include "PioneerQ38FullOffloadVectorQ4Role.h"
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_BATCHED
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
#include "PioneerQ38FullOffloadMetricsRole.h"
#else
#if PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1 || PIONEER_DRIVER_BUILD_TAG != 0x2026110400010089ULL
#error Full offload requires exact tag89 for its distinct batched firmware
#endif
#endif
#else
#if PIONEER_DRIVER_BUILD_TAG != 0x2026110400010088ULL || defined(PIONEER_Q38_FULL_OFFLOAD_METRICS)
#error Full offload requires exact tag88 for its original firmware
#endif
#endif
#ifndef PIONEER_Q38_FULL_OFFLOAD_METRICS_BINDING_H
#define PIONEER_Q38_FULL_OFFLOAD_METRICS_BINDING_H
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
enum {PioneerQ38FullOffloadMetricsSelection=1};
#else
enum {PioneerQ38FullOffloadMetricsSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
static_assert(PioneerQ38FullOffloadMetricsSelection==1,"late full-card metrics selection");
#else
static_assert(PioneerQ38FullOffloadMetricsSelection==0,"late full-card metrics removal");
#endif
#ifndef PIONEER_Q38_FULL_OFFLOAD_FIRMWARE_BINDING_H
#define PIONEER_Q38_FULL_OFFLOAD_FIRMWARE_BINDING_H
#ifdef PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL
enum {PioneerQ38FullOffloadFirmwareSelection=6};
#elif defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4)
enum {PioneerQ38FullOffloadFirmwareSelection=5};
#elif defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4)
enum {PioneerQ38FullOffloadFirmwareSelection=4};
#elif defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4)
enum {PioneerQ38FullOffloadFirmwareSelection=3};
#elif defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED)
enum {PioneerQ38FullOffloadFirmwareSelection=2};
#else
enum {PioneerQ38FullOffloadFirmwareSelection=1};
#endif
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_BATCHED
#ifdef PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL
static_assert(PioneerQ38FullOffloadFirmwareSelection==6,"late full-card prepared counter Q4 selection");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4)
static_assert(PioneerQ38FullOffloadFirmwareSelection==5,"late full-card prepared Q4 selection");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4)
static_assert(PioneerQ38FullOffloadFirmwareSelection==4,"late full-card macro Q4 selection");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4)
static_assert(PioneerQ38FullOffloadFirmwareSelection==3,"late full-card vector Q4 selection");
#else
static_assert(PioneerQ38FullOffloadFirmwareSelection==2,"late full-card batched selection");
#endif
#else
static_assert(PioneerQ38FullOffloadFirmwareSelection==1,"late full-card batched removal");
#endif
#if defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC) || defined(__FAST_MATH__)
#error Full offload cannot alias a diagnostic,CPU reference,hybrid Work or client role
#endif
#if defined(api_version) || defined(init_hardware) || defined(init_driver) || defined(uninit_driver) || defined(publish_devices) || defined(find_device)
#error Full offload requires conventional driver entries
#endif
#ifndef PIONEER_Q38_TOKEN_NATIVE_ROLE_KIND_H
#define PIONEER_Q38_TOKEN_NATIVE_ROLE_KIND_H
enum {PioneerQ38TokenNativePhysicalSelection=3};
#endif
static_assert(PioneerQ38TokenNativePhysicalSelection==3,"late full-offload role change");
#ifndef PIONEER_Q38_TOKEN_NATIVE_PROFILE_BINDING_H
#define PIONEER_Q38_TOKEN_NATIVE_PROFILE_BINDING_H
enum {PioneerQ38TokenNativeProfile=3};
#endif
static_assert(PioneerQ38TokenNativeProfile==3,"full offload requires actual G3 programs");
#ifdef PIONEER_Q38_TOKEN_FOOTPRINT_H
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
static_assert(PioneerQ38TokenL1::Family==0x33505451 && PioneerQ38TokenL1::PreparedQ4==1
 && PioneerQ38TokenL1::SavedBytes==53922024,"prepared ROOT code must be journaled through6e000");
#else
static_assert(PioneerQ38TokenL1::Family==0x33505451 && PioneerQ38TokenL1::SavedBytes==53913832,
 "full offload retains the whole checked G3 journal");
#endif
#endif
