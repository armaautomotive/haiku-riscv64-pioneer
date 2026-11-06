/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_BF16_FULL_PROFILE
#ifdef _KERNEL_MODE
#include "PioneerQ38Bf16FullNativeRole.h"
#else
#include "../../../../../tools/pioneer/PioneerQ38Bf16FullClientRole.h"
#endif
#else
#include "PioneerQ38FullOffloadMetricsRole.h"
#endif
