/* SPDX-License-Identifier: MIT */
// Recheck late role changes. This is an unprivileged TEST scenario, never
// a kernel/Work/image entry or a source of native producer/cleanup authority.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) || defined(PIONEER_ATTENTION_FREESTANDING)
#error Original prefix native TEST cannot enter kernel,CPU oracle,image or Work roles,including defined0
#endif
#if defined(PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST) && PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST != 1
#error Exact optional original prefix native TEST selection required
#endif
#ifndef PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST_H
#define PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST_H
#include "../PioneerQ38RecurrentOriginalWeightBytes.h"
#include "../PioneerQ38RecurrentPrefixComparison.h"
// Four pinned images:zero,QPE1,QPX1,QMW1. Actual observations publish ONLY
// after full same-owner native RUN/STOP/120holds/restore/fresh final-END.
int ExecutePrefixOriginalNativeScenario(const char* const*,RecurrentBlockRawLearnedBytes&,uint32_t*,unsigned,PrefixOriginalStageSink* =nullptr);
#endif
