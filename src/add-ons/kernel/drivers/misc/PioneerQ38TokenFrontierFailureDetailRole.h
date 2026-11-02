/* SPDX-License-Identifier: MIT */
// Rechecked private diagnostic selection. This is NOT a new hardware route.
#ifndef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_SELECTION_H
#define PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE
enum {PioneerQ38TokenFrontierFailureDetailSelection=1};
#else
enum {PioneerQ38TokenFrontierFailureDetailSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE
#if PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE != 1
#error Exact failure-detail revision required
#endif
static_assert(PioneerQ38TokenFrontierFailureDetailSelection==1,"late failure-detail selection");
// Includes all exact kernel/private/renamed-ABI parents and refuses public
// diagnostic,USER,shader,CPU oracle and Work aliases. Failed public010c
// remains retired; this selector does not authorize installing it again.
#include "PioneerQ38TokenTwinBatchPrefetchNativeRole.h"
// The native-context discriminator alone does not check every DATA parent
// (for example FAULT_PROGRAMS). Require the actual complete program role,
// not merely enough flags to name an owner type.
#include "../../../../../tools/pioneer/PioneerQ38TokenTwinBatchPrefetchProgramsRole.h"
#else
static_assert(PioneerQ38TokenFrontierFailureDetailSelection==0,"late failure-detail removal");
#endif
