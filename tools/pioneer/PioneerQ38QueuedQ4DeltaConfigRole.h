/* SPDX-License-Identifier: MIT */
// NEW offboard candidate. Never aliases the running queued-E package.
#include "PioneerQ38QueuedQ4BlockRole.h"
#if !defined(PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_IMAGE) || PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_IMAGE != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4) || defined(PIONEER_Q38_QUEUED_Q4_PAIR_CONFIG_IMAGE)
#error Delta-config Q4 IMAGE requires its distinct offboard subject,not the full-model queued-E target
#endif
