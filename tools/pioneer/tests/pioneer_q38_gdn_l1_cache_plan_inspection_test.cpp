/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_L1_CACHE_PLAN_INSPECTION 1
#include "../PioneerQ38GdnL1CachePlanInspection.h"
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
	std::printf("PASS inspection-only recurrent-state L1 map: %u half-states, %u MiB selected state, %u/%u slots per %u workers, minimum scratch %u bytes; layer tasks collision-free; NOT runtime fit or performance proof\n",
		P::TotalHalves,P::TotalStateBytes/(1024*1024),P::BaseSlots,P::MaxSlots,P::Workers,P::MinimumScratch);
}
