/* SPDX-License-Identifier: MIT */
// Recheck on every include. New immutable package, not an old diagnostic
// role promoted to full inference by aliasing a namespace or its flags.
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h"
#if !defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) || PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1
#error Full offload batched DATA requires its actual tag89 native role
#endif
#if defined(PIONEER_Q38_FULL_OFFLOAD_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS) || defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS) || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS) || defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE) || defined(PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE)
#error Full offload batched DATA cannot borrow old firmware,diagnostic selectors or shader authority
#endif
static_assert(PioneerQ38FullOffloadFirmwareSelection==2,"actual new full-card batched package binding");
