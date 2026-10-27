/* SPDX-License-Identifier: MIT */
// Recheck late/defined0 role changes before the ordinary type guard.
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE)
#error Offline raw-byte/native-scenario TEST boundary is not kernel/CPU-oracle/image/Work authority,including defined0
#endif
#if defined(PIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST) && PIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST != 1
#error Exact optional original-native TEST selection required,including defined0
#endif
#ifndef PIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST_H
#define PIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST_H
#include "../PioneerQ38OriginalWeightBytes.h"
// Six pinned image directories:zero,QPE1,ROOT,worker,QDP,QDA. Receives
// ORIGINAL BYTES only,never an expected input/result/reference or source
// grant. Output is published ONLY after genuine capture+restore+fresh END.
// A physical validation client separately compares BEFORE explicit END;
// this offline simulated scenario is NOT that driver/client or a token.
int ExecuteMixedOriginalNativeScenario(const char* const*,ProjectionExecutionLearnedBytes&,uint32_t*,unsigned);
#endif
