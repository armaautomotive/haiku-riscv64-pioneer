/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_BATCH_PREFETCH_PLAN_H
#define PIONEER_QWEN35_BATCH_PREFETCH_PLAN_H

// The persistent peer uses two disjoint 16 KiB slots for complete original
// quantized batches. The ROOT matrix image has a different private map and
// must never enable this helper.
static inline __UINT32_TYPE__ PioneerQwen35BatchPrefetchBuffer(unsigned ordinal)
{
	return 0x98000U+((ordinal&1U)<<14);
}

#endif
