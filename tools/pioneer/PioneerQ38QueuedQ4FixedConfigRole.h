/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QueuedQ4BlockRole.h"
#if !defined(PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE) || PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE != 1 \
 || defined(PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_IMAGE) || defined(PIONEER_Q38_QUEUED_Q4_PAIR_CONFIG_IMAGE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4)
#error Fixed-config Q4 IMAGE requires its distinct offboard subject,not delta/pair or full-model queued-E
#endif
