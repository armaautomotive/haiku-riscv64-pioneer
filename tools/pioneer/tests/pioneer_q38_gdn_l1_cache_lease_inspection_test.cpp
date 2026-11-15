/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_L1_CACHE_LEASE_INSPECTION 1
#include "../PioneerQ38GdnL1CacheLeaseInspection.h"
#define PIONEER_Q38_GDN_L1_CACHE_CAPACITY_INSPECTION 1
#include "../PioneerQ38GdnL1CacheCapacityInspection.h"
#define PIONEER_Q38_GDN_L1_CACHE_PLAN_INSPECTION 1
#include "../PioneerQ38GdnL1CachePlanInspection.h"
#define PIONEER_Q38_GDN_L1_PAGE_TABLE_PLAN_INSPECTION 1
#include "../PioneerQ38GdnL1PageTablePlanInspection.h"
#include <cassert>
#include <cstdio>

namespace C=PioneerQ38GdnL1CacheCapacityInspection;
using P=PioneerQ38GdnL1RedistributedPlanInspection;
using T=PioneerQ38GdnL1PageTablePlanInspection;
static C::FragmentedSlot pages[P::Workers][C::MaxSlots];
static unsigned pageCounts[P::Workers]={};

static unsigned RunToken(PioneerQ38GdnL1CacheLeaseInspection& lease,
	const PioneerQ38GdnIdentity& id,uint64_t position)
{
	assert(lease.BeginToken(id,position));unsigned hits=0,residentMisses=0,spills=0;
	for(unsigned ordinal=0;ordinal<P::RecurrentLayers;ordinal++){
		const unsigned layer=PioneerQ38GdnTaskPlacement::ActualLayer(ordinal);
		PioneerQ38GdnTicket ticket={};assert(lease.BeginLayer(layer,ticket));
		for(unsigned logical=0;logical<P::LogicalWorkers;logical++){
			PioneerQ38GdnL1CacheSlice current={},next={};
			assert(lease.Slice(ticket,logical,false,current) && lease.Slice(ticket,logical,true,next));
			assert(current.state.route.physicalWorker==next.state.route.physicalWorker
				&& current.state.route.stateIndex==next.state.route.stateIndex
				&& current.cacheIndex==next.cacheIndex && current.resident==next.resident);
			const unsigned worker=current.state.route.physicalWorker-1;
			assert(worker<P::Workers && current.state.route.stateIndex<P::SlotsForPhysical(worker+1));
			assert(current.resident==(current.state.route.stateIndex<pageCounts[worker]));
			T::Entry pageTable={};
			assert(T::Build(ordinal,logical/2,logical%2,pages,pageCounts,pageTable));
			assert(pageTable.resident==current.resident && pageTable.cacheIndex==current.cacheIndex
				&& pageTable.route.physicalWorker==current.state.route.physicalWorker
				&& pageTable.route.peer==current.state.route.peer
				&& pageTable.route.stateIndex==current.state.route.stateIndex);
			if(current.resident){
				for(unsigned page=0;page<C::PagesPerState;page++)
					assert(C::Free(worker+1,pages[worker][current.state.route.stateIndex].pages[page],C::PageBytes));
				for(unsigned page=0;page<C::PagesPerState;page++)
					assert(pageTable.pages[page]==pages[worker][current.state.route.stateIndex].pages[page]);
				if(current.cacheHit)hits++;else residentMisses++;
			}else{assert(!current.cacheHit);for(unsigned page=0;page<C::PagesPerState;page++)assert(!pageTable.pages[page]);spills++;}
			// Reject a completion receipt inconsistent with the selected path,
			// then accept the correctly evidenced metadata transition.
			assert(!lease.CompleteHalf(ticket,logical,!current.resident));
			assert(lease.CompleteHalf(ticket,logical,current.resident));
			if(current.resident){
				PioneerQ38GdnL1CacheSlice uncommitted={};
				assert(lease.Slice(ticket,logical,false,uncommitted) && !uncommitted.cacheHit);
			}
			P::Slot route={};assert(P::Map(ordinal,logical/2,logical%2,route));
			assert(route.physicalWorker==current.state.route.physicalWorker
				&& route.peer==current.state.route.peer && route.index==current.state.route.stateIndex);
		}
		assert(lease.CompleteLayer(ticket));
	}
	assert(lease.CommitToken(id,position));
	assert(hits+residentMisses+spills==P::TotalHalves);
	return hits;
}

int main()
{
	for(unsigned worker=0;worker<P::Workers;worker++){
		unsigned pagesForWorker=0;
		assert(C::BuildFragmented(worker+1,pages[worker],C::MaxSlots,pageCounts[worker],pagesForWorker));
		assert(pageCounts[worker]==25 && pagesForWorker==202);
	}
	const PioneerQ38GdnIdentity id={0x11,0x22,0x33,0x44};
	const PioneerQ38GdnIdentity wrong={0x11,0x22,0x33,0x45};
	PioneerQ38GdnL1CacheLeaseInspection lease;assert(lease.Bind(id));
	assert(!lease.BeginToken(wrong,0));
	assert(RunToken(lease,id,0)==0); // cold L1: every resident entry must be filled
	for(uint64_t position=1;position<4;position++)
		assert(RunToken(lease,id,position)==2975); // exact warm hits; 1633 spill
	assert(!lease.Unsafe());
	// Leave a token incomplete after mutating some cache pages. Fault poisons
	// the lease and invalidates every entry; no partial token can retain hits.
	assert(lease.BeginToken(id,4));PioneerQ38GdnTicket ticket={};assert(lease.BeginLayer(0,ticket));
	for(unsigned logical=0;logical<8;logical++){
		PioneerQ38GdnL1CacheSlice slice={};assert(lease.Slice(ticket,logical,false,slice) && slice.cacheHit);
		assert(lease.CompleteHalf(ticket,logical,slice.resident));
	}
	lease.Fault();assert(lease.Unsafe());
	assert(!lease.BeginToken(id,4) && !lease.Close());
	std::puts("PASS inspection-only GDN L1 cache lease: cold fill, 2,975 warm resident hits, 1,633 deterministic GDDR spills, 48-layer/96-task commit gating, failed receipts rejected, partial-token fault invalidates cache; NOT hardware ownership, transfer proof, Work/native path or performance");
}
