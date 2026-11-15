/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnL1PageAllocator.h"

static PioneerQ38GdnL1PageAllocator sAllocator;

extern "C" bool
pioneer_q38_gdn_l1_page_allocator_compile_witness()
{
	PioneerQ38GdnTaskPlacement::Route route={};
	PioneerQ38GdnL1PageAllocator::PageTable table={};
	return sAllocator.Build()
		&& PioneerQ38GdnTaskPlacement::Map(0,0,route)
		&& sAllocator.Resolve(route,table)
		&& table.resident;
}
