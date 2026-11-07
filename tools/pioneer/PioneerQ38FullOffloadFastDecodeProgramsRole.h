/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadFastDecodeRole.h"
#if defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_PROFILE_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS_H) || defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE)
#error Fast decode profile requires its complete distinct firmware package
#endif
static_assert(PioneerQ38FullOffloadFirmwareSelection==12,"exact fast decode firmware required");
