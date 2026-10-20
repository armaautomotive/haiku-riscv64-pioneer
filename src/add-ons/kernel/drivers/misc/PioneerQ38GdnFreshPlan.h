/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_FRESH_PLAN_H
#define PIONEER_Q38_GDN_FRESH_PLAN_H
#include "PioneerQ38GdnBanks.h"
// CPU-ONLY private proposal,NOT an allocator/reservation/initialization or
// IO/compute authority. No native operation selects it. Allocate this
// object on the HEAP:private staging deliberately exceeds kernel stack.
// Fixed public27B shape:48 recurrent layers x2 slots x48 complete heads.
// Stripe EACH layer/slot's48 heads into7 groups of6/7 heads,rotating the
// preferred banks across layers/slots. Avoid a one-bank state bottleneck.
// Bounded112-page contiguous groups;failure is NOT proof that a finer
// fragmented alternative could not fit. Never increase7x3GiB limits.
class PioneerQ38GdnFreshPlan {
	PioneerQ38GdnRegion regions[672]={};
	unsigned cursor[7]={};
	bool attempted=false,ready=false;
	static constexpr unsigned BankPages=unsigned(PioneerResidentBanks::Capacity/4096);
	bool Find(const uint8_t* initialized,unsigned bank,unsigned pages,unsigned& first) {
		unsigned run=0;
		while (cursor[bank]) {
			unsigned page=--cursor[bank];
			if (initialized[uint64_t(bank)*BankPages+page]) run=0;
			else if (++run==pages) { first=page; return true; }
		}
		return false;
	}
public:
	PioneerQ38GdnFreshPlan()=default;
	PioneerQ38GdnFreshPlan(const PioneerQ38GdnFreshPlan&)=delete;
	PioneerQ38GdnFreshPlan& operator=(const PioneerQ38GdnFreshPlan&)=delete;
	bool Build(const uint8_t* initialized,unsigned pages) {
		if (attempted) return false;
		attempted=true;
		uintptr_t input=reinterpret_cast<uintptr_t>(initialized),self=reinterpret_cast<uintptr_t>(this),limit=~uintptr_t(0);
		if (!initialized || pages!=PioneerResidentBanks::Pages || input>limit-pages || self>limit-sizeof(*this)
			|| !(input+pages<=self || self+sizeof(*this)<=input)) return false;
		// A corrupt bitmap must not be treated as free memory. Caller holds
		// the SAME exclusive context lock throughout;actual Bind MUST still
		// independently recheck EVERY proposed fresh page before reservation.
		for (unsigned page=0;page<pages;page++) if (initialized[page]>1) return false;
		for (auto& next:cursor) next=BankPages;
		unsigned index=0;
		for (unsigned layer=0;layer<64;layer++) if (layer%4!=3) for (unsigned slot=0;slot<2;slot++) {
			unsigned head=0;
			for (unsigned stripe=0;stripe<7;stripe++,index++) {
				unsigned heads=6+unsigned(stripe<6),preferred=(layer-layer/4+slot+stripe)%7;
				bool found=false;
				for (unsigned candidate=0;candidate<7;candidate++) {
					unsigned bank=(preferred+candidate)%7,first=0;
					if (!Find(initialized,bank,heads*16,first)) continue;
					regions[index]={layer,slot,head,heads,bank,first*4096}; found=true; break;
				}
				if (!found) return false; // partial staging NEVER becomes visible
				head+=heads;
			}
		}
		ready=index==672; return ready;
	}
	const PioneerQ38GdnRegion* Regions() const { return ready?regions:nullptr; }
	unsigned Count() const { return ready?672:0; }
};
static_assert(sizeof(PioneerQ38GdnFreshPlan)>=672*sizeof(PioneerQ38GdnRegion) && sizeof(PioneerQ38GdnFreshPlan)<20480,"HEAP-ONLY bounded private proposal");
#endif
