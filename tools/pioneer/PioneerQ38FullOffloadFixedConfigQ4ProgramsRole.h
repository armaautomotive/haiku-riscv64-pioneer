/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadFixedConfigQ4Role.h"
#if defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_PROFILE_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_COUNTER_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS_H) || defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE)
#error Full-card fixed-config Q4 requires exact ROOT+worker bytes,not an old package/image alias
#endif
static_assert(PioneerQ38FullOffloadFirmwareSelection==11,"exact fixed-config full-model firmware required");
