/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadSpillQ4Role.h"
#if defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PROGRAMS_H) || defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE)
#error Spill Q4 DATA cannot alias an older ROOT or program package
#endif
static_assert(PioneerQ38FullOffloadFirmwareSelection==13,"exact tag100 spill Q4 DATA binding");
