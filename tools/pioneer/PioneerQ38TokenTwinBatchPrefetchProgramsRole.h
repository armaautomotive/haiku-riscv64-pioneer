/* SPDX-License-Identifier: MIT */
// Pure DATA only. Do not reuse an old native owner just because packet
// counts match. Exact ROM,buffer lifetime and full END need qualification.
#include "PioneerQ38TokenTwinBatchPrefetchDataBoundary.h"
#ifndef PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PARENT_BINDING_H
#define PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PARENT_BINDING_H
#ifdef PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS
enum {PioneerQ38TokenTwinBatchPrefetchParentBinding=1};
#else
enum {PioneerQ38TokenTwinBatchPrefetchParentBinding=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenTwinBatchPrefetchNativeRole.h"
#include "PioneerQ38TokenBatchedPrefetchProgramsRole.h"
static_assert(PioneerQ38TokenTwinBatchPrefetchDataSelection==2,"exact private twin-batch binding");
#endif
