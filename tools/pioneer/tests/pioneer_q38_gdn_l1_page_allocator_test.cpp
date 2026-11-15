/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>

#define PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4 1
#define PIONEER_Q38_FULL_OFFLOAD_BATCHED 1
#define PIONEER_Q38_FULL_OFFLOAD_METRICS 1
#define PIONEER_Q38_FULL_OFFLOAD_CLIENT 1
#define PIONEER_Q38_GDN_L1_CACHE_CAPACITY_INSPECTION 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnL1PageAllocator.h"
#include "../PioneerQ38GdnL1CacheCapacityInspection.h"

int main()
{
	PioneerQ38GdnL1PageAllocator allocator;
	assert(!allocator.Ready() && allocator.Build() && allocator.Ready()
		&& !allocator.Build());
	uint32_t canonicalPages[PioneerQ38GdnL1CacheCapacityInspection::MaxPages];
	unsigned resident=0,spill=0;
	for(unsigned layer=0;layer<PioneerQ38GdnTaskPlacement::RecurrentLayers;layer++){
		bool seen[PioneerQ38GdnTaskPlacement::PhysicalWorkers]={};
		for(unsigned logical=0;logical<PioneerQ38GdnTaskPlacement::LogicalWorkers;logical++){
			PioneerQ38GdnTaskPlacement::Route route={};
			assert(PioneerQ38GdnTaskPlacement::Map(layer,logical,route));
			assert(!seen[route.physicalWorker-1]);seen[route.physicalWorker-1]=true;
			PioneerQ38GdnL1PageAllocator::PageTable table={};
			assert(allocator.Resolve(route,table));
			assert(table.physicalWorker==route.physicalWorker
				&& table.stateIndex==route.stateIndex
				&& table.cacheIndex==(route.physicalWorker-1)*39+route.stateIndex);
			if(table.resident){
				unsigned canonicalCount=0;
				assert(PioneerQ38GdnL1CacheCapacityInspection::BuildPages(
					route.physicalWorker,canonicalPages,
					PioneerQ38GdnL1CacheCapacityInspection::MaxPages,canonicalCount));
				assert(canonicalCount>=34*8);
				resident++;
				for(unsigned page=0;page<8;page++){
					assert(table.pages[page]
						==canonicalPages[route.stateIndex*8+page]);
					assert((table.pages[page]&4095)==0
						&& table.pages[page]<=0x17f000);
					for(unsigned prior=0;prior<page;prior++)
						assert(table.pages[prior]!=table.pages[page]);
					for(unsigned span=0;span<PioneerQ38TokenPushedL1::SnapshotSpans(route.physicalWorker);span++){
						const auto s=PioneerQ38TokenPushedL1::SnapshotSpan(route.physicalWorker,span);
						assert(table.pages[page]+4096<=s.address
							|| s.address+s.words*4<=table.pages[page]);
					}
				}
			}else{
				spill++;
				for(unsigned page=0;page<8;page++)assert(!table.pages[page]);
			}
			PioneerQ38GdnTaskPlacement::Route bad=route;
			bad.physicalWorker=bad.physicalWorker%119+1;
			PioneerQ38GdnL1PageAllocator::PageTable untouched={};
			assert(!allocator.Resolve(bad,untouched)
				&& untouched.physicalWorker==0);
		}
	}
	assert(resident==4046 && spill==562);
	PioneerQ38GdnL1PageAllocator::PageTable out={};
	assert(!allocator.Resolve(0,0,out) && !allocator.Resolve(120,0,out)
		&& !allocator.Resolve(1,39,out));
	std::printf("PASS prepared-Q4 QTP3 page allocator: resident=%u spill=%u 119-peer routes verified; metadata only, no L1 allocation/I/O/token authority\n",resident,spill);
}
