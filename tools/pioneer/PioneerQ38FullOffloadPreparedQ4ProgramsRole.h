/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadPreparedQ4Role.h"
#if defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE) || defined(PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE)
#error Prepared Q4 DATA cannot alias old program/diagnostic/image selectors
#endif
static_assert(PioneerQ38FullOffloadFirmwareSelection==5,"prepared source requires its exact paired firmware");
