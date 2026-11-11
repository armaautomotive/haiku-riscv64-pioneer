/* SPDX-License-Identifier: MIT */
// QGWF ROOT/plan inspection only; the fake-tag plan can compile native code.
#include "PioneerQ38QgwfNativeInspectionRole.h"
#if defined(PIONEER_Q38_QGWF_DUAL_BATCH_ROOT) && (PIONEER_Q38_QGWF_DUAL_BATCH_ROOT != 1 \
 || !defined(PIONEER_Q38_QGWE_BATCH_DENSE_ROOT) || PIONEER_Q38_QGWE_BATCH_DENSE_ROOT != 1 \
 || defined(PIONEER_Q38_QGWF_DUAL_BATCH_PLAN) || defined(PIONEER_Q38_QGWF_BATCH4_WORKER) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGWF ROOT requires its isolated QGWE parent image role
#endif
#if defined(PIONEER_Q38_QGWF_DUAL_BATCH_PLAN) && (PIONEER_Q38_QGWF_DUAL_BATCH_PLAN != 1 \
 || !defined(PIONEER_Q38_QGWE_BATCH_DENSE_PLAN) || PIONEER_Q38_QGWE_BATCH_DENSE_PLAN != 1 \
 || defined(PIONEER_Q38_QGWF_DUAL_BATCH_ROOT) || defined(PIONEER_Q38_QGWF_BATCH4_WORKER) \
 || ((defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG)) && !defined(PIONEER_Q38_QGWF_NATIVE_INSPECTION)) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGWF plan requires its isolated QGWE parent plan role
#endif
#ifndef PIONEER_Q38_QGWF_ROOT_BINDING_H
#define PIONEER_Q38_QGWF_ROOT_BINDING_H
#if defined(PIONEER_Q38_QGWF_DUAL_BATCH_ROOT)
enum {PioneerQ38QgwfRootSelection=1};
#elif defined(PIONEER_Q38_QGWF_DUAL_BATCH_PLAN)
enum {PioneerQ38QgwfRootSelection=2};
#else
enum {PioneerQ38QgwfRootSelection=0};
#endif
#endif
#if defined(PIONEER_Q38_QGWF_DUAL_BATCH_ROOT)
static_assert(PioneerQ38QgwfRootSelection==1,"late QGWF root selection");
#elif defined(PIONEER_Q38_QGWF_DUAL_BATCH_PLAN)
static_assert(PioneerQ38QgwfRootSelection==2,"late QGWF plan selection");
#else
static_assert(PioneerQ38QgwfRootSelection==0,"late QGWF root/plan removal");
#endif
