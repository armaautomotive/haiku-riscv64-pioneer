/* SPDX-License-Identifier: MIT */
// Separate full-model package,never the offboard fixed-config IMAGE role.
#include "PioneerQ38FullOffloadQueuedQ4Role.h"
#if !defined(PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4) || PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4 != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE) \
 || defined(PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_IMAGE) || defined(PIONEER_Q38_QUEUED_Q4_PAIR_CONFIG_IMAGE)
#error Full-card fixed-config Q4 requires its distinct complete-model package,not queued-E or offboard images
#endif
#ifdef _KERNEL_MODE
#if PIONEER_DRIVER_BUILD_TAG != 0x2026110500010103ULL
#error Full-card fixed-config Q4 requires exact tag103 native parent
#endif
#endif
