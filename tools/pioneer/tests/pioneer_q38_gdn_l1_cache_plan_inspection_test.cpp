/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_L1_CACHE_PLAN_INSPECTION 1
#define PIONEER_Q38_GDN_L1_CACHE_CAPACITY_INSPECTION 1
#include "../PioneerQ38GdnL1CachePlanInspection.h"
#include "../PioneerQ38GdnL1CacheCapacityInspection.h"
#include "../PioneerQ38GdnState.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnShape.h"
#include <cassert>
#include <cstdio>

int main()
{
	using P=PioneerQ38GdnL1CachePlanInspection;
	static_assert(P::TotalHalves==4608,"48 recurrent layers x 48 heads x two FP32 half-states");
	static_assert(P::TotalStateBytes==144U*1024*1024,"one selected current state slot per model state");
	static_assert(P::MinimumScratch==0,"48 current GDN workers each need 48 half-states,leaving no L1 scratch");
	unsigned counts[P::Workers]={};
	bool occupied[P::Workers][P::MaxSlots]={};
	for(unsigned l=0;l<P::RecurrentLayers;l++){
		assert(P::IsRecurrentLayer(P::ActualLayer(l)) && P::LayerWorkersUnique(l));
		if(l+1<P::RecurrentLayers)assert(P::ActualLayer(l+1)>P::ActualLayer(l));
		bool owners[P::Workers]={};
		for(unsigned h=0;h<P::HeadsPerLayer;h++)for(unsigned half=0;half<2;half++){
			P::Slot s={};assert(P::Map(l,h,half,s));
			assert(s.ordinal==l*P::HalvesPerLayer+h*2+half && s.worker==s.ordinal%P::Workers);
			assert(s.index==s.ordinal/P::Workers && s.offset==s.index*P::HalfStateBytes);
			assert(s.offset%P::HalfStateBytes==0 && s.offset+s.bytes<=P::L1Bytes);
			assert(!occupied[s.worker][s.index]);occupied[s.worker][s.index]=true;counts[s.worker]++;owners[s.worker]=true;
		}
		unsigned ownerCount=0;for(bool value:owners)ownerCount+=value;assert(ownerCount==P::HalvesPerLayer);
	}
	unsigned total=0,high=0,low=0;
	for(unsigned worker=0;worker<P::Workers;worker++){
		assert(counts[worker]==P::SlotsForWorker(worker));
		assert(P::ScratchBytes(worker)==P::L1Bytes-counts[worker]*P::HalfStateBytes);
		assert(P::ScratchBytes(worker)>=P::MinimumScratch);
		if(worker<P::ExtraSlots)high++;else low++;
		total+=counts[worker];
	}
	assert(total==P::TotalHalves && high==P::ExtraSlots && low==P::Workers-P::ExtraSlots);
	P::Slot invalid={};assert(!P::Map(P::RecurrentLayers,0,0,invalid));
	assert(!P::Map(0,P::HeadsPerLayer,0,invalid));assert(!P::Map(0,0,2,invalid));
	assert(!P::LayerWorkersUnique(P::RecurrentLayers));assert(P::SlotsForWorker(P::Workers)==0);
	assert(P::ActualLayer(47)==62 && P::ActualLayer(48)==64 && !P::IsRecurrentLayer(63));
	namespace C=PioneerQ38GdnL1CacheCapacityInspection;
	unsigned availableSlots=0,deficit=0;
	for(unsigned worker=0;worker<P::Workers;worker++){
		C::Slot freeSlots[C::MaxSlots]={};unsigned count=0;
		assert(C::Build(worker+1,freeSlots,C::MaxSlots,count));
		assert(count==31 && count<P::SlotsForWorker(worker));
		for(unsigned i=0;i<count;i++){
			assert(freeSlots[i].bytes==P::HalfStateBytes && C::Free(worker+1,freeSlots[i].address,freeSlots[i].bytes));
			for(unsigned j=0;j<i;j++)assert(freeSlots[i].address>=freeSlots[j].address+freeSlots[j].bytes
				|| freeSlots[j].address>=freeSlots[i].address+freeSlots[i].bytes);
		}
		availableSlots+=count;deficit+=P::SlotsForWorker(worker)-count;
	}
	assert(availableSlots==2976 && deficit==1632);
	unsigned fragmentedSlots=0,fragmentedPages=0,fragmentedMisses=0;
	C::FragmentedSlot fragmentedPlacement[P::Workers][C::MaxSlots]={};
	unsigned fragmentedCount[P::Workers]={};
	for(unsigned worker=0;worker<P::Workers;worker++){
		unsigned count=0,pages=0;
		assert(C::BuildFragmented(worker+1,fragmentedPlacement[worker],C::MaxSlots,count,pages));
		assert(count==34 && pages==274 && count<P::SlotsForWorker(worker));
		bool pageUsed[C::MaxPages]={};
		for(unsigned i=0;i<count;i++)for(unsigned j=0;j<C::PagesPerState;j++){
			const auto address=fragmentedPlacement[worker][i].pages[j];assert(!(address&(C::PageBytes-1)) && C::Free(worker+1,address,C::PageBytes));
			const unsigned page=address/C::PageBytes;assert(page<C::MaxPages && !pageUsed[page]);pageUsed[page]=true;
		}
		fragmentedCount[worker]=count;fragmentedSlots+=count;fragmentedPages+=pages;fragmentedMisses+=P::SlotsForWorker(worker)-count;
	}
	assert(fragmentedSlots==3264 && fragmentedPages==26304 && fragmentedMisses==1344);
	bool mapped[P::Workers][P::MaxSlots]={};unsigned residentStates=0,spilledStates=0;
	for(unsigned layer=0;layer<P::RecurrentLayers;layer++)for(unsigned head=0;head<P::HeadsPerLayer;head++)for(unsigned half=0;half<2;half++){
		P::Slot state={};assert(P::Map(layer,head,half,state));assert(!mapped[state.worker][state.index]);mapped[state.worker][state.index]=true;
		if(state.index<fragmentedCount[state.worker]){
			for(unsigned page=0;page<C::PagesPerState;page++)
				assert(C::Free(state.worker+1,fragmentedPlacement[state.worker][state.index].pages[page],C::PageBytes));
			residentStates++;
		}else spilledStates++;
	}
	assert(residentStates==fragmentedSlots && spilledStates==fragmentedMisses);
	for(unsigned worker=0;worker<P::Workers;worker++)for(unsigned index=0;index<P::MaxSlots;index++)
		assert(mapped[worker][index]==(index<P::SlotsForWorker(worker)));
	PioneerQ38GdnState runtimeState;const PioneerQ38GdnIdentity identity={1,1,1,1};
	assert(runtimeState.Bind(identity) && runtimeState.BeginToken(identity,0));
	unsigned runtimeResident=0,runtimeSpilled=0;
	for(unsigned layer=0;layer<P::RecurrentLayers;layer++){
		const unsigned actualLayer=P::ActualLayer(layer);assert(P::IsRecurrentLayer(actualLayer));
		PioneerQ38GdnTicket ticket={};assert(runtimeState.BeginLayer(actualLayer,ticket));
		for(unsigned worker=0;worker<P::Workers;worker++){
			PioneerQ38GdnStateSlice prior={},next={};
			assert(runtimeState.Slice(ticket,worker,false,prior) && runtimeState.Slice(ticket,worker,true,next));
			assert(prior.head==worker/2 && prior.half==worker%2 && prior.firstWord==worker*8192 && prior.words==8192);
			assert(next.head==prior.head && next.half==prior.half && next.slot==(prior.slot^1));
			P::Slot placement={};assert(P::Map(layer,prior.head,prior.half,placement));
			assert(placement.worker==worker && placement.index==layer);
			if(placement.index<fragmentedCount[worker]){
				for(unsigned page=0;page<C::PagesPerState;page++)
					assert(C::Free(worker+1,fragmentedPlacement[worker][placement.index].pages[page],C::PageBytes));
				runtimeResident++;
			}else runtimeSpilled++;
			assert(runtimeState.CompletedHalf(ticket,worker));
		}
		assert(runtimeState.CompleteLayer(ticket));
	}
	assert(runtimeResident==fragmentedSlots && runtimeSpilled==fragmentedMisses);
	assert(runtimeState.CommitToken(identity,0));
	using R=PioneerQ38GdnL1RedistributedPlanInspection;
	C::FragmentedSlot redistributedPlacement[R::Workers][C::MaxSlots]={};
	unsigned redistributedCount[R::Workers]={},redistributedPages=0;
	bool peerSeen[1024]={};
	for(unsigned worker=0;worker<R::Workers;worker++){
		unsigned pages=0;assert(C::BuildFragmented(worker+1,redistributedPlacement[worker],C::MaxSlots,redistributedCount[worker],pages));
		assert(redistributedCount[worker]==34 && pages==274);redistributedPages+=pages;
		unsigned peer=PioneerQ38MixedWorkerLayout::Peer(worker+1);assert(peer<1024 && !peerSeen[peer]);peerSeen[peer]=true;
	}
	for(unsigned worker=0;worker<P::Workers;worker++)
		assert(PioneerQ38ResidentGdnPeer(worker)==PioneerQ38GdnTaskPlacement::Peer(worker+1));
	unsigned redistributedLoads[R::Workers]={};bool redistributedSlots[R::Workers][C::MaxSlots]={};
	unsigned redistributedResident=0,redistributedSpill=0;
	PioneerQ38GdnState remappedState;
	assert(remappedState.Bind(identity) && remappedState.BeginToken(identity,0));
	for(unsigned layer=0;layer<R::RecurrentLayers;layer++){
		const unsigned actualLayer=P::ActualLayer(layer);
		PioneerQ38GdnTicket ticket={};assert(remappedState.BeginLayer(actualLayer,ticket));
		bool activePeer[R::Workers]={};
		for(unsigned head=0;head<R::HeadsPerLayer;head++)for(unsigned half=0;half<2;half++){
			const unsigned logical=head*2+half;
			PioneerQ38GdnRedistributedStateSlice view={};
			assert(remappedState.RedistributedSlice(ticket,logical,false,view));
			R::Slot slot={};assert(R::Map(layer,head,half,slot));
			assert(view.state.layer==actualLayer && view.state.head==head && view.state.half==half);
			assert(view.route.physicalWorker==slot.physicalWorker && view.route.peer==slot.peer
				&& view.route.stateIndex==slot.index && view.route.logicalWorker==logical);
			const unsigned worker=slot.physicalWorker-1;assert(worker<R::Workers && !activePeer[worker]);activePeer[worker]=true;
			assert(slot.logicalWorker==logical && slot.bytes==P::HalfStateBytes);
			assert(slot.peer==PioneerQ38MixedWorkerLayout::Peer(slot.physicalWorker));
			assert(slot.index==redistributedLoads[worker]++ && slot.index<R::SlotsForPhysical(slot.physicalWorker));
			assert(!redistributedSlots[worker][slot.index]);redistributedSlots[worker][slot.index]=true;
			if(slot.index<redistributedCount[worker]){
				for(unsigned page=0;page<C::PagesPerState;page++)
					assert(C::Free(slot.physicalWorker,redistributedPlacement[worker][slot.index].pages[page],C::PageBytes));
				redistributedResident++;
			}else redistributedSpill++;
			assert(remappedState.CompletedHalf(ticket,logical));
		}
		assert(remappedState.CompleteLayer(ticket));
		for(unsigned physical=1;physical<=R::Workers;physical++){
			unsigned logical=UINT32_MAX;
			const bool active=PioneerQ38GdnTaskPlacement::LogicalWorker(PioneerQ38GdnTaskPlacement::ActualLayer(layer),physical,logical);
			assert(active==activePeer[physical-1]);
			if(active){R::Slot slot={};assert(R::Map(layer,logical/2,logical%2,slot) && slot.physicalWorker==physical);}
		}
	}
	for(unsigned worker=0;worker<R::Workers;worker++){
		assert(redistributedLoads[worker]==R::SlotsForPhysical(worker+1));
		assert(redistributedLoads[worker]==38 || redistributedLoads[worker]==39);
		for(unsigned i=0;i<redistributedLoads[worker];i++)assert(redistributedSlots[worker][i]);
	}
	assert(redistributedResident==4046 && redistributedSpill==562 && redistributedPages==32606);
	assert(remappedState.CommitToken(identity,0));
	unsigned invalidOrdinal=0,invalidLogical=0;
	assert(!PioneerQ38GdnTaskPlacement::LayerOrdinal(63,invalidOrdinal));
	assert(!PioneerQ38GdnTaskPlacement::LogicalWorker(63,1,invalidLogical));
	assert(!PioneerQ38GdnTaskPlacement::LogicalWorker(0,0,invalidLogical));
	assert(!PioneerQ38GdnTaskPlacement::LogicalWorker(0,120,invalidLogical));
	std::printf("PASS inspection-only recurrent-state L1 map: %u half-states, %u MiB selected state, %u/%u slots per %u current GDN workers, required scratch %u bytes; layer tasks collision-free; NOT runtime fit or performance proof\n",
		P::TotalHalves,P::TotalStateBytes/(1024*1024),P::BaseSlots,P::MaxSlots,P::Workers,P::MinimumScratch);
	std::printf("PASS QTP3 untouched-range capacity check: %u of %u 32KiB recurrent-state slots fit across %u current GDN peer L1s (%u bytes); %u slots (%u bytes) need relocation; each peer has 31 safe slots vs %u required; NO all-phase live map or performance proof\n",
		availableSlots,P::TotalHalves,P::Workers,availableSlots*P::HalfStateBytes,deficit,deficit*P::HalfStateBytes,P::BaseSlots);
	std::printf("PASS 4KiB-scattered placement: %u pages mapped %u current-runtime state halves (%u bytes) across one 48-layer token; %u spill to GDDR; each GDN worker keeps 34 layers (2 page fragments) and spills 14; 110 new pages per worker needed for full capacity; GDN chunk-import address indirection NOT implemented or measured\n",
		fragmentedPages,runtimeResident,runtimeResident*P::HalfStateBytes,runtimeSpilled);
	std::printf("PASS experimental 119-peer redistribution map: %u logical tasks/layer use unique peer routes; state loads balance 38/39 per peer, with %u resident and %u GDDR-spill halves; existing GDN runtime rejects this remap and no address-indirect cache loader exists\n",
		R::LogicalWorkers,redistributedResident,redistributedSpill);
}
