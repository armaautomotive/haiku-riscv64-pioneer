/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
#error QGW3 receiving ROOT is not a precise worker image role
#endif
#ifndef PIONEER_Q38_PRECISE_WORKER_BINDING_H
#define PIONEER_Q38_PRECISE_WORKER_BINDING_H
#ifdef PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE
enum {PioneerQ38PreciseWorkerPushProfile=1};
#else
enum {PioneerQ38PreciseWorkerPushProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE
#include "PioneerQ38PushedWorkerImageRole.h"
static_assert(PioneerQ38PreciseWorkerPushProfile==1,"late QGW3 image policy change");
#else
static_assert(PioneerQ38PreciseWorkerPushProfile==0,"late QGW2 image policy change");
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION
#error Worker-sealed admission is a returning ROOT policy,not a worker image
#endif
// No include guard: late and defined-zero contamination must fail.
#if !defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || PIONEER_Q38_PRECISE_WORKER_IMAGE != 1
#error Exact separate QGW2 precise worker image required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT) \
 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) \
 || defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGW2 image is separate from kernel, old worker, ROOT, CPU reference or Work
#endif
