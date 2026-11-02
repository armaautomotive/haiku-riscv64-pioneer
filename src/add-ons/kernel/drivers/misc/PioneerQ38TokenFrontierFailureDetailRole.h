/* SPDX-License-Identifier: MIT */
// Rechecked private OR separately bound public diagnostic selection.
// Neither syntax nor BUILD qualification authorizes installation/execution.
#if defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_BENCHMARK_NATIVE) && !defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE)
#error Public failure-detail binding requires its actual observer selection
#endif
#ifndef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_SELECTION_H
#define PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_BENCHMARK_NATIVE
enum {PioneerQ38TokenFrontierFailureDetailSelection=2};
#else
enum {PioneerQ38TokenFrontierFailureDetailSelection=1};
#endif
#else
enum {PioneerQ38TokenFrontierFailureDetailSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE
#if PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE != 1
#error Exact failure-detail revision required
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_BENCHMARK_NATIVE
static_assert(PioneerQ38TokenFrontierFailureDetailSelection==2,"late public failure-detail selection");
#include "PioneerQ38TokenFrontierFailureDetailBenchmarkRole.h"
#else
static_assert(PioneerQ38TokenFrontierFailureDetailSelection==1,"late private failure-detail selection");
// Includes all exact kernel/private/renamed-ABI parents and refuses public
// diagnostic,USER,shader,CPU oracle and Work aliases. Failed public010c
// remains retired; this selector does not authorize installing it again.
#include "PioneerQ38TokenTwinBatchPrefetchNativeRole.h"
// The native-context discriminator alone does not check every DATA parent
// (for example FAULT_PROGRAMS). Require the actual complete program role,
// not merely enough flags to name an owner type.
#include "../../../../../tools/pioneer/PioneerQ38TokenTwinBatchPrefetchProgramsRole.h"
#endif
#else
static_assert(PioneerQ38TokenFrontierFailureDetailSelection==0,"late failure-detail removal");
#endif
