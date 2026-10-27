/* SPDX-License-Identifier: MIT */
// Neutral TEST byte boundary,not a model/reference/driver capability.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) || defined(PIONEER_ATTENTION_FREESTANDING)
#error Original recurrent TEST bytes cannot enter kernel,CPU-oracle,image or Work roles,including defined0
#endif
#if defined(PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST) && PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST != 1
#error Exact optional original recurrent TEST selection required
#endif
#ifndef PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST_H
#define PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST_H
#include "../PioneerQ38RecurrentOriginalWeightBytes.h"
#include "../PioneerQ38RecurrentBlockComparison.h"
// Four pinned image directories:zero,QPE1,QRB1,QMW1. Only raw original
// learned bytes cross into this TU. Captured simulated GDDR output is
// published ONLY after complete native STOP/holds/restore/fresh END.
// NOT a physical readback/driver,CPU reference,full-model token or Work.
int ExecuteRecurrentOriginalNativeScenario(const char* const*,RecurrentBlockRawLearnedBytes&,uint32_t*,unsigned,RecurrentOriginalStageSink* =nullptr);
#endif
