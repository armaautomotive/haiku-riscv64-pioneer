/* SPDX-License-Identifier: MIT */
#ifndef HAIKU_PIONEER_MEMORY_H
#define HAIKU_PIONEER_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* Separate the low, contiguous loader allocator from kernel-visible RAM.
 * This is private to an explicitly requested Pioneer memory trial.
 */
struct PioneerMemoryRange {
	uint64_t start;
	uint64_t size;
};

struct PioneerMemoryPlan {
	PioneerMemoryRange ranges[32];
	unsigned count;
};

static inline int
PioneerMemoryLimitValid(uint64_t gib)
{
	return gib == 8 || gib == 16 || gib == 32 || gib == 64 || gib == 128;
}

static inline int
PioneerMemoryPlanInit(PioneerMemoryPlan& plan, uint64_t gib)
{
	plan.count = 0;
	if (!PioneerMemoryLimitValid(gib))
		return 0;
	plan.ranges[0] = {0x2200000, (gib << 30) - 0x2200000};
	plan.count = 1;
	return 1;
}

/* Exclude whole pages intersecting a reservation. Reject malformed/overflowed
 * intervals, including ones outside the selected trial's memory ceiling.
 * A valid reservation wholly outside the plan has no effect.
 */
static inline int
PioneerMemoryExclude(PioneerMemoryPlan& plan, uint64_t start, uint64_t size)
{
	if (!size || size > UINT64_MAX - start
		|| start + size > UINT64_MAX - 4095)
		return 0;
	uint64_t end = (start + size + 4095) & ~UINT64_C(4095);
	start &= ~UINT64_C(4095);
	for (unsigned i = 0; i < plan.count;) {
		PioneerMemoryRange& range = plan.ranges[i];
		uint64_t rangeEnd = range.start + range.size;
		if (end <= range.start || start >= rangeEnd) {
			i++;
			continue;
		}
		if (start <= range.start && end >= rangeEnd) {
			for (unsigned j = i + 1; j < plan.count; j++)
				plan.ranges[j - 1] = plan.ranges[j];
			plan.count--;
		} else if (start <= range.start) {
			range.start = end;
			range.size = rangeEnd - end;
			i++;
		} else if (end >= rangeEnd) {
			range.size = start - range.start;
			i++;
		} else {
			if (plan.count == 32)
				return 0;
			for (unsigned j = plan.count; j > i + 1; j--)
				plan.ranges[j] = plan.ranges[j - 1];
			plan.ranges[i + 1] = {end, rangeEnd - end};
			range.size = start - range.start;
			plan.count++;
			i += 2;
		}
	}
	return 1;
}

static inline int
PioneerMemoryContains(const PioneerMemoryPlan& plan, uint64_t start,
	uint64_t size)
{
	if (!size || size > UINT64_MAX - start)
		return 0;
	for (unsigned i = 0; i < plan.count; i++) {
		const PioneerMemoryRange& range = plan.ranges[i];
		if (start >= range.start && start - range.start < range.size
			&& size <= range.size - (start - range.start))
			return 1;
	}
	return 0;
}

#endif
