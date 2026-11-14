/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_L1_CACHE_PLAN_INSPECTION 1
#define PIONEER_Q38_GDN_L1_CACHE_CAPACITY_INSPECTION 1
#include "../PioneerQ38GdnL1CachePlanInspection.h"
#include "../PioneerQ38GdnL1CacheCapacityInspection.h"
#include <cassert>
#include <cstdio>

int main()
{
	using P=PioneerQ38GdnL1CachePlanInspection;
	static_assert(P::TotalHalves==4608,"48 recurrent layers x 48 heads x two FP32 half-states");
	static_assert(P::TotalStateBytes==144U*1024*1024,"one selected current state slot per model state");
	static_assert(P::MinimumScratch==294912,"39 half-states leave 288 KiB per fullest worker");
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
		assert(P::ScratchBytes(worker)>=P::MinimumScratch-P::HalfStateBytes);
		if(counts[worker]==P::MaxSlots)high++;else low++;
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
	assert(availableSlots==3689 && deficit==919);
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
	assert(fragmentedSlots==4046 && fragmentedPages==32606 && fragmentedMisses==562);
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
	std::printf("PASS inspection-only recurrent-state L1 map: %u half-states, %u MiB selected state, %u/%u slots per %u workers, minimum scratch %u bytes; layer tasks collision-free; NOT runtime fit or performance proof\n",
		P::TotalHalves,P::TotalStateBytes/(1024*1024),P::BaseSlots,P::MaxSlots,P::Workers,P::MinimumScratch);
	std::printf("PASS QTP3 untouched-range capacity check: %u of %u 32KiB recurrent-state slots fit across %u peer L1s (%u bytes); %u slots (%u bytes) need scratch relocation; each peer has 31 safe slots vs %u/%u required; NO all-phase live map or performance proof\n",
		availableSlots,P::TotalHalves,P::Workers,availableSlots*P::HalfStateBytes,deficit,deficit*P::HalfStateBytes,P::BaseSlots,P::MaxSlots);
	std::printf("PASS 4KiB-scattered placement: %u pages yielded %u bijectively mapped resident states (%u bytes); %u states explicitly spill to GDDR; every peer has 34 from 274 pages (2 residual); 4/5 more states need 30/38 additional pages; GDN chunk-import address indirection NOT implemented or measured\n",
		fragmentedPages,residentStates,residentStates*P::HalfStateBytes,spilledStates);
}
