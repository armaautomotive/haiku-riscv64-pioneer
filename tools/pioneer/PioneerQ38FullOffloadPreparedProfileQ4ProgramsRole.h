/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadPhaseProfileRole.h"
#if defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_COUNTER_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS_H) || defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE)
#error Full-card phase profile requires its distinct paired firmware,not old package/image aliases
#endif
static_assert(PioneerQ38FullOffloadFirmwareSelection==7,"exact full-model phase-profile firmware required");
