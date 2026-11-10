/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QgwdNativeInspectionRole.h"
// DATA selection only, not an image/native/producer capability.
#ifdef PIONEER_Q38_TOKEN_BF16_PROGRAMS
#if PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION)
#error Exact QGW4 worker DATA cannot alias integer/pushed DATA or BUILD selectors
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU
#include "PioneerQ38Bf16RootRole.h"
#endif
#ifdef PIONEER_Q38_TOKEN_PUSHED_PROGRAMS
#if PIONEER_Q38_TOKEN_PUSHED_PROGRAMS != 1 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION)
#error Exact QTP3 worker DATA cannot alias QTP2 or BUILD-image selectors
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
#if PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) \
 || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS)
#error Distinct QGW3 ROOT selection requires precise BUILD,not fixed QTP2 DATA
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
#if PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1
#error Exact separate precise-activation ROOT profile required
#endif
#endif
#if defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) && PIONEER_Q38_TOKEN_PRECISE_PROGRAMS != 1
#error Exact separate QTP2 program DATA profile required
#endif
#ifndef PIONEER_Q38_SELECTED_GLOBAL_WORKER_H
#define PIONEER_Q38_SELECTED_GLOBAL_WORKER_H
#if defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION)
#if PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP) \
 || PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) \
 || PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU != 1 || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS)
#error QGW8 ROOT requires its exact non-loadable hybrid BF16 inspection profile
#endif
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
#include "PioneerQ38DenseQ8MatrixWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38DenseQ8MatrixWorkerLayout;
#else
#include "PioneerQ38HybridHeadWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38HybridHeadWorkerLayout;
#endif
enum {PioneerQ38SelectedWorkerBootEnd=51,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=0,PioneerQ38SelectedWorkerBf16=1,PioneerQ38SelectedWorkerHybrid=1};
#elif defined(PIONEER_Q38_QGWD_PROGRAMS_INSPECTION)
#if PIONEER_Q38_QGWD_PROGRAMS_INSPECTION != 1 || !defined(PIONEER_Q38_QGWD_PLAN_SIMULATION) \
 || PIONEER_Q38_QGWD_PLAN_SIMULATION != 1 || !defined(PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION) \
 || PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION != 1 || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) \
 || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 || !defined(PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION) \
 || PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION != 1 || !defined(PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC) \
 || !defined(PIONEER_Q38_HYBRID_PROGRAMS_HEADER) || !defined(PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC) \
 || !defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1 \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP) || defined(PIONEER_Q38_QGWC_PROGRAMS_INSPECTION) \
 || defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) \
 || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error QGWD worker DATA requires isolated matching program and expanded journal roles
#endif
#include "PioneerQ38DenseQ8MatrixWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38DenseQ8MatrixWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=51,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=0,PioneerQ38SelectedWorkerBf16=1,PioneerQ38SelectedWorkerHybrid=1};
#elif defined(PIONEER_Q38_QGWC_PROGRAMS_INSPECTION)
#if PIONEER_Q38_QGWC_PROGRAMS_INSPECTION != 1 || (!defined(PIONEER_Q38_QGWC_PLAN_SIMULATION) && !defined(PIONEER_Q38_QGWC_NATIVE_136)) \
 || (defined(PIONEER_Q38_QGWC_PLAN_SIMULATION) && PIONEER_Q38_QGWC_PLAN_SIMULATION != 1) \
 || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC) || PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_HYBRID_PROGRAMS_HEADER) || !defined(PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC) \
 || PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) \
 || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1 || defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP) \
 || (defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) && !defined(PIONEER_Q38_QGWC_NATIVE_136))
#error QGWC worker DATA requires isolated program and plan simulation roles
#endif
#include "PioneerQ38HybridHeadWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38HybridHeadWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=51,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=0,PioneerQ38SelectedWorkerBf16=1,PioneerQ38SelectedWorkerHybrid=1};
#elif defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION)
#if PIONEER_Q38_QGW8_PROGRAMS_INSPECTION != 1 || !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) \
 || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) \
 || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 || !defined(PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC) \
 || PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_HYBRID_PROGRAMS_HEADER) \
 || !defined(PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC) \
 || PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) \
 || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1 || defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP)
#error QGW8 worker DATA requires its exact non-loadable program and layout inspection profile
#endif
#include "PioneerQ38HybridHeadWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38HybridHeadWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=51,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=0,PioneerQ38SelectedWorkerBf16=1,PioneerQ38SelectedWorkerHybrid=1};
#elif defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP)
#if PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) \
 || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS)
#error Hybrid ROOT requires a distinct experimental BF16 build, not an installed program selector
#endif
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#include "PioneerQ38HybridWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38HybridWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=51,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=0,PioneerQ38SelectedWorkerBf16=1,PioneerQ38SelectedWorkerHybrid=1};
#elif defined(PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC)
#if PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC) \
 || PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) \
 || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1
#error Hybrid QGW5 startup DATA requires the exact offline hybrid journal and program selectors
#endif
#include "PioneerQ38HybridWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38HybridWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=51,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=0,PioneerQ38SelectedWorkerBf16=1,PioneerQ38SelectedWorkerHybrid=1};
#elif defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS)
#include "PioneerQ38Bf16WorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38Bf16WorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=49,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=0,PioneerQ38SelectedWorkerBf16=1,PioneerQ38SelectedWorkerHybrid=0};
#elif defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS)
#include "PioneerQ38PushedWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38PushedWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=55,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=1,PioneerQ38SelectedWorkerBf16=0,PioneerQ38SelectedWorkerHybrid=0};
#elif defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS)
#include "PioneerQ38PreciseWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38PreciseWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=49,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=0,PioneerQ38SelectedWorkerBf16=0,PioneerQ38SelectedWorkerHybrid=0};
#else
#include "PioneerQ38GlobalWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38GlobalWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=46,PioneerQ38SelectedWorkerPrecise=0,PioneerQ38SelectedWorkerPushed=0,PioneerQ38SelectedWorkerBf16=0,PioneerQ38SelectedWorkerHybrid=0};
#endif
#endif
#if defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP) || defined(PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC)
static_assert(PioneerQ38SelectedWorkerHybrid==1,"late hybrid ROOT selection");
#else
static_assert(PioneerQ38SelectedWorkerHybrid==0,"late hybrid ROOT removal");
#endif
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS)
static_assert(PioneerQ38SelectedWorkerPushed==1,"late QGW3 DATA selection");
#else
static_assert(PioneerQ38SelectedWorkerPushed==0,"late old worker DATA selection");
#endif
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS)
static_assert(PioneerQ38SelectedWorkerPrecise==1,"late worker DATA profile change");
#else
static_assert(PioneerQ38SelectedWorkerPrecise==0,"late worker DATA profile change");
#endif
#if defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS)
static_assert(PioneerQ38SelectedWorkerBf16==1,"late BF16 worker DATA profile change");
#else
static_assert(PioneerQ38SelectedWorkerBf16==0,"late BF16 worker DATA profile change");
#endif
