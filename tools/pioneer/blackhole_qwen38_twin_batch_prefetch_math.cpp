/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TWIN_BATCH_PARENT_BINDING 1
#include "PioneerQ38TokenTwinBatchPrefetchRole.h"
#include "PioneerQ38TokenBatchedPrefetchRole.h"
static_assert(PioneerQ38TokenTwinBatchPrefetchSelection==1
 && PioneerQ38TokenBatchedPrefetchSelection==1,"distinct twin-batch math TU cannot fall back");
#include "blackhole_qwen38_resident_operator_service.cpp"
