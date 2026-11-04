/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadMacroQ4Role.h"
#if defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED_PROGRAMS_H) || defined(PIONEER_Q38_FULL_OFFLOAD_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS) || defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE) \
 || defined(PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE) || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS)
#error Full-card macro Q4 DATA cannot alias an old package or shader authority
#endif
static_assert(PioneerQ38FullOffloadFirmwareSelection==4,"exact full-card macro Q4 native binding");
