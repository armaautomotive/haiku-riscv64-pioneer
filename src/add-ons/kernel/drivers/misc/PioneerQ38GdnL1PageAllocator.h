/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_L1_PAGE_ALLOCATOR_H
#define PIONEER_Q38_GDN_L1_PAGE_ALLOCATOR_H

#if !defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4) \
	|| PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4 != 1
#error GDN L1 page map is bound to the prepared-Q4 QTP3 image footprint
#endif

#include "PioneerQ38GdnTaskPlacement.h"
#include "../../../../../tools/pioneer/PioneerQ38TokenPushedFootprint.h"

// Deterministic page map for the prepared-Q4 QTP3 image. This is heap-only
// native metadata: it derives addresses outside the complete per-tile saved
// L1 image, but does not itself map SRAM, transfer state, or prove a token.
class PioneerQ38GdnL1PageAllocator {
public:
	enum : unsigned {
		Workers=PioneerQ38GdnTaskPlacement::PhysicalWorkers,
		LogicalTasks=PioneerQ38GdnTaskPlacement::TotalTasks,
		SlotsPerWorker=PioneerQ38GdnTaskPlacement::SlotsPerPhysical,
		ResidentSlotsPerWorker=34,
		PagesPerSlot=PioneerQ38GdnTaskPlacement::StateHalfBytes/4096,
		L1Bytes=0x180000,
		PageBytes=4096,
		ResidentSlots=Workers*ResidentSlotsPerWorker,
		SpillSlots=LogicalTasks-ResidentSlots
	};
	struct PageTable {
		unsigned physicalWorker;
		unsigned stateIndex;
		unsigned cacheIndex;
		bool resident;
		uint32_t pages[PagesPerSlot];
	};

private:
	uint32_t fPages[Workers][ResidentSlotsPerWorker][PagesPerSlot] = {};
	bool fReady=false;

	static bool Free(unsigned worker,uint32_t address)
	{
		if(!worker || worker>PioneerQ38TokenPushedL1::Workers-1
			|| (address&(PageBytes-1)) || address>L1Bytes-PageBytes)
			return false;
		for(unsigned i=0;i<PioneerQ38TokenPushedL1::SnapshotSpans(worker);i++){
			const auto span=PioneerQ38TokenPushedL1::SnapshotSpan(worker,i);
			const uint32_t bytes=span.words*4;
			if(address<span.address+bytes && span.address<address+PageBytes)
				return false;
		}
		return true;
	}

	static bool Collect(unsigned worker,uint32_t* pages,unsigned capacity,unsigned& count)
	{
		count=0;
		if(!pages || capacity> L1Bytes/PageBytes || !worker
			|| worker>PioneerQ38TokenPushedL1::Workers-1)
			return false;
		for(uint32_t address=0;address<L1Bytes;address+=PageBytes){
			if(!Free(worker,address))continue;
			if(count>=capacity)return false;
			pages[count++]=address;
		}
		return true;
	}

public:
	PioneerQ38GdnL1PageAllocator()=default;
	PioneerQ38GdnL1PageAllocator(const PioneerQ38GdnL1PageAllocator&)=delete;
	PioneerQ38GdnL1PageAllocator& operator=(const PioneerQ38GdnL1PageAllocator&)=delete;

	bool Build()
	{
		if(fReady)return false;
		for(unsigned worker=1;worker<=Workers;worker++){
			uint32_t available[L1Bytes/PageBytes];
			unsigned count=0;
			if(!Collect(worker,available,L1Bytes/PageBytes,count)
				|| count<ResidentSlotsPerWorker*PagesPerSlot)
				return false;
			for(unsigned slot=0;slot<ResidentSlotsPerWorker;slot++)
				for(unsigned page=0;page<PagesPerSlot;page++)
					fPages[worker-1][slot][page]
						=available[slot*PagesPerSlot+page];
		}
		fReady=true;
		return true;
	}

	bool Ready()const{return fReady;}

	bool Resolve(unsigned physicalWorker,unsigned stateIndex,PageTable& output)const
	{
		if(!fReady || !physicalWorker || physicalWorker>Workers
			|| stateIndex>=SlotsPerWorker)
			return false;
		PageTable candidate={physicalWorker,stateIndex,
			(physicalWorker-1)*SlotsPerWorker+stateIndex,
			stateIndex<ResidentSlotsPerWorker,{}};
		if(candidate.resident){
			for(unsigned page=0;page<PagesPerSlot;page++){
				const uint32_t address=fPages[physicalWorker-1][stateIndex][page];
				if(!Free(physicalWorker,address))return false;
				for(unsigned prior=0;prior<page;prior++)
					if(candidate.pages[prior]==address)return false;
				candidate.pages[page]=address;
			}
		}
		output=candidate;
		return true;
	}

	bool Resolve(const PioneerQ38GdnTaskPlacement::Route& route,PageTable& output)const
	{
		PioneerQ38GdnTaskPlacement::Route expected={};
		if(!PioneerQ38GdnTaskPlacement::Map(route.layerOrdinal,
			route.logicalWorker,expected)
			|| expected.actualLayer!=route.actualLayer
			|| expected.physicalWorker!=route.physicalWorker
			|| expected.peer!=route.peer
			|| expected.stateIndex!=route.stateIndex)
			return false;
		return Resolve(route.physicalWorker,route.stateIndex,output);
	}
};

static_assert(sizeof(PioneerQ38GdnL1PageAllocator)>129000
	&& sizeof(PioneerQ38GdnL1PageAllocator)<130000,
	"full QTP3 scattered recurrent-state page map is heap-only");
static_assert(PioneerQ38GdnL1PageAllocator::ResidentSlots==4046
	&& PioneerQ38GdnL1PageAllocator::SpillSlots==562,
	"exact 119-peer cache/spill geometry");

#endif
