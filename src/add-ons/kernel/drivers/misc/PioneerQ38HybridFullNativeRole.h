/* SPDX-License-Identifier: MIT */
// Distinct candidate only: build is not numerical or hardware qualification.
#include "../../../../../tools/pioneer/PioneerQ38HybridKernelCompileOnly.h"
#if !defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || PIONEER_Q38_HYBRID_FULL_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_EMIT_CONTEXT_BODY) || PIONEER_Q38_TOKEN_EMIT_CONTEXT_BODY != 1 \
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
 || !defined(TT_PIONEER_DRIVER_RELOAD) || TT_PIONEER_DRIVER_RELOAD != 1
#error Hybrid full native requires all fixed token, bank and transport parents
#endif
#if defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC) \
 || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) || defined(PIONEER_Q38_FULL_OFFLOAD_METRICS) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_RESIDENT) || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4) || defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL) || defined(PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE) || defined(PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4) || defined(PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY) || defined(PIONEER_Q38_BF16_FULL_PROFILE) \
 || defined(PIONEER_Q38_BF16_PAIR_ISSUE_FULL) || defined(PIONEER_Q38_BF16_DEFERRED_FULL) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_BF16_WORKER_IMAGE) || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE) || defined(__FAST_MATH__)
#error Hybrid full native cannot alias old, image-build, validation or alternate driver roles
#endif
#if defined(api_version) || defined(init_hardware) || defined(init_driver) || defined(uninit_driver) || defined(publish_devices) || defined(find_device)
#error Hybrid full native requires conventional driver entries
#endif
#ifndef PIONEER_Q38_TOKEN_NATIVE_ROLE_KIND_H
#define PIONEER_Q38_TOKEN_NATIVE_ROLE_KIND_H
#ifdef PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128
enum {PioneerQ38TokenNativePhysicalSelection=8};
#elif defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127)
enum {PioneerQ38TokenNativePhysicalSelection=7};
#else
enum {PioneerQ38TokenNativePhysicalSelection=5};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128
static_assert(PioneerQ38TokenNativePhysicalSelection==8,"distinct QGW8 candidate subject required");
#elif defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127)
static_assert(PioneerQ38TokenNativePhysicalSelection==7,"distinct matrix candidate subject required");
#else
static_assert(PioneerQ38TokenNativePhysicalSelection==5,"distinct hybrid native subject required");
#endif
#ifndef PIONEER_Q38_TOKEN_NATIVE_PROFILE_BINDING_H
#define PIONEER_Q38_TOKEN_NATIVE_PROFILE_BINDING_H
#ifdef PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128
enum {PioneerQ38TokenNativeProfile=8};
#elif defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127)
enum {PioneerQ38TokenNativeProfile=7};
#else
enum {PioneerQ38TokenNativeProfile=5};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128
static_assert(PioneerQ38TokenNativeProfile==8,"QGW8 candidate token program family required");
#elif defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127)
static_assert(PioneerQ38TokenNativeProfile==7,"QGW7 candidate token program family required");
#else
static_assert(PioneerQ38TokenNativeProfile==5,"QGW5 token program family required");
#endif
#ifdef PIONEER_Q38_TOKEN_FOOTPRINT_H
static_assert(PioneerQ38TokenL1::Family==0x35505451 && PioneerQ38TokenL1::SavedBytes==137812488,
 "hybrid native needs its complete expanded arbitrary-original journal");
#endif
