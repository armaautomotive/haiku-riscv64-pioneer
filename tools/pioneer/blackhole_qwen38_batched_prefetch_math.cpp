/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenBatchedPrefetchRole.h"
static_assert(PioneerQ38TokenTwinBatchPrefetchSelection==0,"old batched leaf cannot become twin-batch math");
static_assert(PioneerQ38TokenBatchedPrefetchSelection==1,"distinct batched math TU cannot fall back to old prefetch");
#include "blackhole_qwen38_resident_operator_service.cpp"
