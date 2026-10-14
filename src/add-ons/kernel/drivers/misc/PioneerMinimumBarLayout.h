/* Distributed under the MIT License. */
#ifndef PIONEER_MINIMUM_BAR_LAYOUT_H
#define PIONEER_MINIMUM_BAR_LAYOUT_H

#include "PioneerPciProfile.h"

static inline bool
PioneerMinimumBarLayoutMatches(const tt_pioneer_profile& p)
{
	if (p.version != 1 || p.stage != 6 || p.status != 0 || p.ready != 1
		|| p.controller != UINT64_C(0x7062000000)
		|| p.config != UINT64_C(0x4800000000) || p.config_length != 0x1000
		|| p.card_bus != 3 || p.root_bus != 2 || p.root_command != 0
		|| p.root_buses != 0x003f0100 || p.root_memory != 0x0000fff0
		|| p.root_prefetch != 0x0001fff1 || p.root_prefetch_base_high != 0
		|| p.root_prefetch_limit_high != 0)
		return false;
	const uint64_t hosts[] = {0x4810800000, 0x4890000000, 0x48c0000000, 0x4900000000, 0x4b00000000};
	const uint64_t addresses[] = {0x800000, 0x90000000, 0xc0000000, 0x4900000000, 0x4b00000000};
	const uint64_t sizes[] = {0x400000, 0x30000000, 0x10000000, 0x200000000, 0x100000000};
	// Haiku io_resource and PCI address-type values, captured in this ABI.
	const uint32_t types[] = {2, 1, 1, 1, 1};
	const uint32_t flags[] = {0, 8, 0, 12, 4};
	for (unsigned i = 0; i < TT_PIONEER_RANGE_COUNT; i++) {
		const tt_pioneer_range& range = p.ranges[i];
		if (range.host != hosts[i] || range.pci != addresses[i]
			|| range.size != sizes[i] || range.type != types[i] || range.flags != flags[i])
			return false;
		// BAR4 is deliberately not forwarded through any host memory range.
		if (range.type == 1 && (range.size == 0
			|| range.pci > UINT64_MAX - (range.size - 1)
			|| range.pci + range.size - 1 >= UINT64_C(0xfffffff800000000)))
			return false;
	}
	return p.bar0_host_start == UINT64_C(0x4900000000)
		&& p.bar0_host_end == UINT64_C(0x491fffffff)
		&& p.bar2_host_start == UINT64_C(0x4920000000)
		&& p.bar2_host_end == UINT64_C(0x49200fffff);
}

#endif
