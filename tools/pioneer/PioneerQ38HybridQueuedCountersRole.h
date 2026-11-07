/* SPDX-License-Identifier: MIT */
// Separate returning-coordinator inspection; no native/device/Work grant.
#ifdef PIONEER_Q38_HYBRID_QUEUED_COUNTERS_IMAGE
#if PIONEER_Q38_HYBRID_QUEUED_COUNTERS_IMAGE != 1 || !defined(__riscv) || __riscv != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 \
 || !defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT) || PIONEER_Q38_LAYER_COMPONENT_RECURRENT != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS) || PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP) || PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP != 1 \
 || !defined(PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION) || PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT) || PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT != 1
#error Hybrid queued counters require the exact QGW7 returning recurrent inspection profile
#endif
#if defined(PIONEER_Q38_COUNTER_GATHER_IMAGE) || defined(PIONEER_Q38_TID_BOUNDARY_IMAGE) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_LAYER_COMPONENT_DENSE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Hybrid queued counters cannot alias other image, native, validation or Work roles
#endif
#endif
#ifndef PIONEER_Q38_HYBRID_QUEUED_COUNTERS_BINDING_H
#define PIONEER_Q38_HYBRID_QUEUED_COUNTERS_BINDING_H
#ifdef PIONEER_Q38_HYBRID_QUEUED_COUNTERS_IMAGE
enum {PioneerQ38HybridQueuedCountersProfile=1};
#else
enum {PioneerQ38HybridQueuedCountersProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_HYBRID_QUEUED_COUNTERS_IMAGE
static_assert(PioneerQ38HybridQueuedCountersProfile==1,"late hybrid queued-counter selection");
#else
static_assert(PioneerQ38HybridQueuedCountersProfile==0,"late hybrid queued-counter removal");
#endif
