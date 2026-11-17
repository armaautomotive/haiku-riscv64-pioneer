/* Distributed under the MIT License. */
#ifndef PIONEER_BLACKHOLE_BAR_PLAN_H
#define PIONEER_BLACKHOLE_BAR_PLAN_H

#include "PioneerPciProfile.h"


struct PioneerBlackholeBarPlan {
	uint64_t hostStart[2];
	uint64_t hostEnd[2];
	uint32_t bars[6];
	uint32_t bridge[3];
};


static inline bool
PioneerRangeEnd(uint64_t start, uint64_t size, uint64_t& end)
{
	if (size == 0 || start > UINT64_MAX - (size - 1))
		return false;
	end = start + (size - 1);
	return true;
}


// Address arithmetic only. The caller still needs the actual endpoint, an
// exclusive root, trusted host windows and verified PCI-to-host translations.
// Place BAR0 and BAR2 in one 64-bit prefetchable aperture. BAR4 remains above
// every reachable PCI memory aperture and must not be forwarded.
static inline bool
PioneerPlanBlackholeBars(const tt_pioneer_range* ranges, unsigned count,
	PioneerBlackholeBarPlan& output)
{
	const uint64_t bar0Size = UINT64_C(0x20000000);
	const uint64_t bar2Size = UINT64_C(0x100000);
	const uint64_t bar4Park = UINT64_C(0xfffffff800000000);
	const uint64_t mask = bar0Size - 1;
	const uint32_t memory = 1;
	const uint32_t ioPorts = 2;
	const uint32_t prefetch64 = 12;
	const uint32_t barFlags = 12;
	if (ranges == nullptr || count == 0 || count > TT_PIONEER_RANGE_COUNT)
		return false;

	uint64_t hostEnd[TT_PIONEER_RANGE_COUNT];
	uint64_t pciEnd[TT_PIONEER_RANGE_COUNT];
	for (unsigned i = 0; i < count; i++) {
		const tt_pioneer_range& range = ranges[i];
		if (range.type != memory && range.type != ioPorts)
			return false;
		if (!PioneerRangeEnd(range.host, range.size, hostEnd[i])
			|| !PioneerRangeEnd(range.pci, range.size, pciEnd[i])
			|| (range.type == memory && pciEnd[i] >= bar4Park))
			return false;
		for (unsigned j = 0; j < i; j++) {
			// No two windows may claim the same host bytes. Separately
			// reject ambiguous translations inside each PCI address space.
			if ((range.host <= hostEnd[j] && ranges[j].host <= hostEnd[i])
				|| (range.type == ranges[j].type && range.pci <= pciEnd[j]
					&& ranges[j].pci <= pciEnd[i]))
				return false;
		}
	}

	PioneerBlackholeBarPlan plan = {};
	bool found = false;
	for (unsigned i = 0; i < count; i++) {
		const tt_pioneer_range& range = ranges[i];
		if (range.type != memory || range.flags != prefetch64
			|| range.pci > UINT64_MAX - mask)
			continue;
		const uint64_t bar0 = (range.pci + mask) & ~mask;
		uint64_t end;
		if (bar0 < UINT64_C(0x100000000)
			|| !PioneerRangeEnd(bar0, bar0Size + bar2Size, end)
			|| end > pciEnd[i])
			continue;
		if (found)
			return false;
		found = true;
		const uint64_t bar2 = bar0 + bar0Size;
		plan.hostStart[0] = range.host + (bar0 - range.pci);
		plan.hostEnd[0] = plan.hostStart[0] + bar0Size - 1;
		plan.hostStart[1] = plan.hostEnd[0] + 1;
		plan.hostEnd[1] = range.host + (end - range.pci);
		// Physical mappings for both BARs have to be page-compatible.
		if ((plan.hostStart[0] & (bar2Size - 1)) != 0)
			return false;
		plan.bars[0] = uint32_t(bar0) | barFlags;
		plan.bars[1] = uint32_t(bar0 >> 32);
		plan.bars[2] = uint32_t(bar2) | barFlags;
		plan.bars[3] = uint32_t(bar2 >> 32);
		plan.bars[4] = uint32_t(bar4Park) | barFlags;
		plan.bars[5] = uint32_t(bar4Park >> 32);
		// PCI type-1 bridge's inclusive 1 MiB-granular prefetch window.
		plan.bridge[0] = uint32_t((bar0 >> 16) & 0xfff0) | 1
			| ((uint32_t((end >> 16) & 0xfff0) | 1) << 16);
		plan.bridge[1] = uint32_t(bar0 >> 32);
		plan.bridge[2] = uint32_t(end >> 32);
	}
	if (!found)
		return false;
	output = plan;
	return true;
}

#endif
