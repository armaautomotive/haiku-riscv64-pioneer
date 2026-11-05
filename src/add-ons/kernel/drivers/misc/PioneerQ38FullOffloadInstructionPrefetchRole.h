/* SPDX-License-Identifier: MIT */
// Rechecked on every include; no implicit old-package or USER/native alias.
#include "PioneerQ38FullOffloadTwoProductReplayRole.h"
#if !defined(PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH) || PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_COUNTER_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_IMAGE)
#error Full-card instruction prefetch requires its distinct whole-model role and paired firmware
#endif
#ifdef _KERNEL_MODE
#if PIONEER_DRIVER_BUILD_TAG != (defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4)?0x2026110500010102ULL:0x2026110500010101ULL)
#error Full-card instruction prefetch requires exact tag101 native parent
#endif
#endif
