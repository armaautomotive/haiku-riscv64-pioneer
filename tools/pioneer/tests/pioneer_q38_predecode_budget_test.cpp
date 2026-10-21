/* SPDX-License-Identifier: MIT */
// OFFLINE immutable-shape estimate,NOT a source parser,allocator or owner.
// Mirror canonical64-row striping and4KiB padding,including existing state
// and workspaces. Never expands the current3GiB/bank ownership boundary.
#include <array>
#include <cassert>
#include <cstdio>
#include <algorithm>
#include "../PioneerQwen38PackedPlan.h"
#include "../PioneerQ38PredecodedQ4.h"
struct Budget {
	std::array<uint64_t,7> used{};
	uint64_t learned=0,total=0;unsigned expanded=0,deltaSlots=1;
	void Stripe(unsigned rows,unsigned rowBytes,unsigned unit,bool inference=false) {
		unsigned counts[7]={},chunks=rows/unit,tail=rows%unit,count=chunks<7?1:7;
		if(count==1)counts[0]=rows;else{for(unsigned b=0;b<7;b++)counts[b]=(chunks/7+unsigned(b<chunks%7))*unit;counts[6]+=tail;}
		unsigned banks[]={0,1,2,3,4,5,6};
		std::sort(banks,banks+7,[this](unsigned a,unsigned b){return used[a]<used[b] || (used[a]==used[b] && a<b);});
		for(unsigned p=0;p<count;p++) {
			uint64_t bytes=(uint64_t(counts[p])*rowBytes+4095)&~UINT64_C(4095);
			used[banks[p]]+=bytes;total+=bytes;if(inference)learned+=bytes;
		}
	}
	bool Fits() const {return *std::max_element(used.begin(),used.end())<=PioneerQwen38PackedPlan::Capacity;}
	static Budget Build(unsigned downCount,bool allQ4=false,bool twoDeltaSlots=false) {
		Budget b;b.deltaSlots=twoDeltaSlots?2:1;unsigned downSeen=0;
		for(unsigned ordinal=0;ordinal<=64;ordinal++)for(unsigned role=0;;role++) {
			PioneerQ38Tensor t;if(!PioneerQ38Expected(ordinal?ordinal-1:64,role,t))break;
			bool inference=t.type && t.rank==2 && t.name!="token_embd.weight";
			unsigned rowBytes=t.type?PioneerQ38TiledRowBytes(t.type,t.inputs):unsigned(t.bytes/t.outputs);
			bool down=t.name.find(".ffn_down.weight")!=std::string::npos;
			bool expand=inference && t.type==12 && (allQ4 || (down && downSeen<downCount));
			if(down)downSeen++;
			if(expand){rowBytes=unsigned(t.inputs/256)*(PioneerQ38PredecodedQ4::Bytes/16);b.expanded++;}
			b.Stripe(unsigned(t.outputs),rowBytes,64,inference);
		}
		for(unsigned layer=0;layer<64;layer++) {
			if(layer%4==3)b.Stripe(8,8192*256*2,1);
			else {for(unsigned slot=0;slot<b.deltaSlots;slot++)b.Stripe(48,128*128*4,1);b.Stripe(10240,12,64);}
		}
		for(unsigned bank=0;bank<7;bank++){b.used[bank]+=64*1024*1024;b.total+=64*1024*1024;}
		return b;
	}
	void Print(const char* mode) const {
		printf("OFFLINE_PREDECODE_BUDGET %s delta_slots%u expanded_tensors%u learned_allocated%llu total_reserved%llu max_bank%llu fits_existing_3GiB_bank_owner%u;NOT_NATIVE_ALLOCATION_OR_SPEED\n",mode,deltaSlots,expanded,(unsigned long long)learned,(unsigned long long)total,(unsigned long long)*std::max_element(used.begin(),used.end()),unsigned(Fits()));
	}
};
int main() {
	const auto control=Budget::Build(0),all=Budget::Build(0,true),down=Budget::Build(64);
	assert(control.Fits() && control.learned==UINT64_C(18672242688) && !control.expanded);
	assert(all.expanded==192 && !all.Fits());assert(down.expanded==64);
	control.Print("original");all.Print("all_learned_Q4");down.Print("all64_ffn_down");
	unsigned maximum=0;for(unsigned count=1;count<=64;count++){auto next=Budget::Build(count);if(next.Fits())maximum=count;}
	assert(maximum && maximum<64);const auto selected=Budget::Build(maximum);assert(selected.expanded==maximum && selected.Fits());
	selected.Print("largest_first_layer_ffn_down_subset");
	unsigned pairedMaximum=0;for(unsigned count=1;count<=64;count++){auto next=Budget::Build(count,false,true);if(next.Fits())pairedMaximum=count;}
	assert(pairedMaximum && pairedMaximum<maximum);Budget::Build(pairedMaximum,false,true).Print("largest_subset_with_double_buffered_delta");
	puts("PASS OFFLINE shape/padding budget reproduces measured original learned allocation;whole-Q4 expansion REFUSED by current owner,selective proposal includes original embedding/state/workspace reservations;NO NEW BANK EXTENT_OR_ADDRESS_AUTHORITY");
}
