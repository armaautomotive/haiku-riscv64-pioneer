/* SPDX-License-Identifier: MIT */
// Re-evaluate on EVERY include;currently DATA only,NOT a native selector.
#include "PioneerQ38TokenBatchedPrefetchDataBoundary.h"
#include "PioneerQ38TokenPrefetchProgramsRole.h"
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS
static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==1,"exact separate batched DATA binding");
#endif
