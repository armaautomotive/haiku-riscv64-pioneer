/* SPDX-License-Identifier: MIT */
// HOST native fixture only. No public selector or installable ABI.
#if !defined(PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_LINK_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_LINK_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_NATIVE) || PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_DETAIL_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_DETAIL_FIXTURE != 1 \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_BENCHMARK_NATIVE)
#error Explicit private QFL1 original-model first-layer/cancel/STOP/HOLD/drain/full-END TEST required
#endif
#include "pioneer_q38_token_native_frontier_failure_detail_role.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenFrontierFailureLinkRole.h"
static_assert(PioneerQ38TokenFrontierFailureLinkSelection==1,"private native QFL1 fixture only");
