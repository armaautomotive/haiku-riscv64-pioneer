/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_GDN_L1_PAGE_TABLE_PLAN_INSPECTION) \
	|| PIONEER_Q38_GDN_L1_PAGE_TABLE_PLAN_INSPECTION != 1
#error Scattered GDN page-table plan is inspection-only
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
	|| defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
	|| defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \
	|| defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(_KERNEL_MODE)
#error Inspection-only page-table plan cannot select Work,validation,or native execution
#endif
#ifndef PIONEER_Q38_GDN_L1_PAGE_TABLE_PLAN_INSPECTION_H
#define PIONEER_Q38_GDN_L1_PAGE_TABLE_PLAN_INSPECTION_H
#define PIONEER_Q38_GDN_L1_CACHE_CAPACITY_INSPECTION 1
#define PIONEER_Q38_GDN_L1_CACHE_PLAN_INSPECTION 1
#include "PioneerQ38GdnL1CacheCapacityInspection.h"
#include "PioneerQ38GdnL1CachePlanInspection.h"

// Map the lease's stable (physical peer,stateIndex) to the exact eight-page
// scattered address table. The caller supplies a separately prepared static
// free-range inventory. This helper performs no allocation, I/O, or writes.
struct PioneerQ38GdnL1PageTablePlanInspection {
	using P=PioneerQ38GdnL1RedistributedPlanInspection;
	enum : unsigned { PagesPerState=PioneerQ38GdnL1CacheCapacityInspection::PagesPerState,
		L1Bytes=PioneerQ38GdnL1CacheCapacityInspection::L1Bytes,
		SlotsPerWorker=PioneerQ38GdnTaskPlacement::SlotsPerPhysical };
	using FragmentedSlot=PioneerQ38GdnL1CacheCapacityInspection::FragmentedSlot;
	struct Entry {
		PioneerQ38GdnTaskPlacement::Route route;
		unsigned cacheIndex;
		bool resident;
		uint32_t pages[PagesPerState];
	};

	static bool Build(unsigned layerOrdinal,unsigned head,unsigned half,
		const FragmentedSlot (&slots)[P::Workers][PioneerQ38GdnL1CacheCapacityInspection::MaxSlots],
		const unsigned (&counts)[P::Workers],
		Entry& output)
	{
		P::Slot mapped={};
		if(!P::Map(layerOrdinal,head,half,mapped) || mapped.physicalWorker==0
			|| mapped.physicalWorker>P::Workers || mapped.index>=P::SlotsForPhysical(mapped.physicalWorker))return false;
		const unsigned worker=mapped.physicalWorker-1;
		if(counts[worker]>PioneerQ38GdnL1CacheCapacityInspection::MaxSlots)return false;
		Entry candidate={};
		candidate.route={layerOrdinal,PioneerQ38GdnTaskPlacement::ActualLayer(layerOrdinal),
			mapped.logicalWorker,mapped.physicalWorker,mapped.peer,mapped.index};
		candidate.cacheIndex=worker*SlotsPerWorker+mapped.index;
		candidate.resident=mapped.index<counts[worker];
		if(candidate.resident){
			for(unsigned page=0;page<PagesPerState;page++){
				const uint32_t address=slots[worker][mapped.index].pages[page];
				if((address&(PioneerQ38GdnL1CacheCapacityInspection::PageBytes-1))
					|| address>L1Bytes-PioneerQ38GdnL1CacheCapacityInspection::PageBytes
					|| !PioneerQ38GdnL1CacheCapacityInspection::Free(mapped.physicalWorker,address,
						PioneerQ38GdnL1CacheCapacityInspection::PageBytes))return false;
				for(unsigned prior=0;prior<page;prior++)
					if(candidate.pages[prior]==address)return false;
				for(unsigned other=0;other<counts[worker];other++)if(other!=mapped.index)
					for(unsigned otherPage=0;otherPage<PagesPerState;otherPage++)
						if(slots[worker][other].pages[otherPage]==address)return false;
				candidate.pages[page]=address;
			}
		}
		output=candidate;return true;
	}
};
#endif
