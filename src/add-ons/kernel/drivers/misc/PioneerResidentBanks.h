/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RESIDENT_BANKS_H
#define PIONEER_RESIDENT_BANKS_H
#include "PioneerResidentSession.h"
#include "BlackholeDramSamplesProbe.h"

// Kernel-private route authority. User space supplies ONLY a logical bank
// ordinal and page offset, never a coordinate, physical bank or NoC address.
// One exclusive store context owns ALL seven enabled banks. Physical bank3
// is harvested out; logical bank3 maps to physical bank4, NOT bank3.
// This is storage, not authorization to launch a model or tensor program.
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
// KERNEL-PRIVATE page ranges, never user pointers/routes/physical addresses.
// Only the experimental GDN owner uses this opt-in reservation interlock.
struct PioneerResidentMutableRange { uint32_t bank,firstPage,pages; };
#endif
class PioneerResidentBanks {
	const tt_dram_samples_result* bound = nullptr;
	uint32_t routes[TT_DRAM_BANKS] = {};
	uint32_t source[TT_DRAM_SOURCE_WORDS] = {};
	bool quarantined = false;
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	const void* mutableOwner=nullptr;
	// Complete recurrent placement (at most672 extents) plus ONE resident
	// FFN workspace. Still ONE sticky reservation owner,never two claims.
	PioneerResidentMutableRange mutableRanges[673]={};
	unsigned mutableCount=0;
	bool MutableOverlap(unsigned bank,uint64_t offset,uint64_t bytes) const {
		// Disjoint ranges sorted once at reservation. Avoid scanning673
		// state extents for every learned matrix worker on every token.
		unsigned first=0,last=mutableCount;
		while (first<last) {
			unsigned middle=first+(last-first)/2; const auto& r=mutableRanges[middle];
			if (r.bank<bank || (r.bank==bank && uint64_t(r.firstPage)*4096<offset+bytes)) first=middle+1;
			else last=middle;
		}
		if (!first) return false;
		const auto& r=mutableRanges[first-1];
		return r.bank==bank && uint64_t(r.firstPage+r.pages)*4096>offset;
	}
#endif
	static bool Proof(const tt_dram_samples_result& baseline) {
		if (baseline.version!=1 || (baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK
			&& baseline.state!=TT_DRAM_SAMPLE_PASSED) || baseline.routes_verified!=7
			|| !baseline.tlb_restored || !baseline.data_restored || baseline.recovery_required)
			return false;
		tt_dram_samples_result rebuilt={};
		for (unsigned w=0;w<TT_DRAM_SOURCE_WORDS;w++) rebuilt.source[w]=baseline.source[w];
		if (!BlackholeDramBuildRoutes(rebuilt)) return false;
		for (unsigned b=0;b<7;b++) {
			unsigned x=rebuilt.translated[b][0],y=rebuilt.translated[b][1];
			if (baseline.translated[b][0]!=x || baseline.translated[b][1]!=y) return false;
			uint32_t physical=BlackholeDramPhysicalX(b)|(BlackholeDramPhysicalY(b)<<6);
			for (unsigned s=0;s<3;s++) {
				const uint32_t* id=baseline.id[b][s];
				if ((id[0]&0xfff)!=physical || ((id[0]>>12)&127)!=17
					|| ((id[0]>>19)&127)!=12 || (id[0]&0xf0000000)!=0x10000000
					|| id[1]!=(0x80000|BlackholeDramBank(b)) || (id[2]&0x9000)!=0x8000
					|| ((id[3]&0xfff)!=physical && (id[3]&0xfff)!=(x|(y<<6)))) return false;
				for (unsigned w=0;w<TT_DRAM_ID_WORDS;w++)
					if (id[w]!=baseline.id[b][0][w]) return false;
			}
		}
		return true;
	}
public:
	static constexpr uint64_t Capacity=UINT64_C(3)*1024*1024*1024;
	static constexpr uint64_t Total=7*Capacity;
	static constexpr uint32_t Base=0x04000000;
	static constexpr uint32_t Pages=uint32_t(Total/4096);
	PioneerResidentBanks() = default;
	PioneerResidentBanks(const PioneerResidentBanks&) = delete;
	PioneerResidentBanks& operator=(const PioneerResidentBanks&) = delete;
	bool Bind(const tt_dram_samples_result& baseline) {
		if (bound || quarantined || !Proof(baseline)) return false;
		for (unsigned w=0;w<TT_DRAM_SOURCE_WORDS;w++) source[w]=baseline.source[w];
		for (unsigned b=0;b<7;b++) routes[b]=baseline.translated[b][0]|(baseline.translated[b][1]<<6);
		bound=&baseline; return true;
	}
	bool Matches(const tt_dram_samples_result& baseline) const {
		if (quarantined || bound!=&baseline || !Proof(baseline)) return false;
		for (unsigned w=0;w<TT_DRAM_SOURCE_WORDS;w++) if (source[w]!=baseline.source[w]) return false;
		for (unsigned b=0;b<7;b++) if (routes[b]!=(baseline.translated[b][0]|(baseline.translated[b][1]<<6))) return false;
		return true;
	}
	void Quarantine() { quarantined=true; } // irreversible until whole context/cold boot ends
	bool Quarantined() const { return quarantined; }
	bool Range(const tt_dram_samples_result& baseline,unsigned bank,uint64_t offset,unsigned pages=1) const {
		return bank<7 && pages>=1 && pages<=4 && !(offset&4095) && offset<Capacity
			&& uint64_t(pages)*4096<=Capacity-offset && Matches(baseline)
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
			&& !MutableOverlap(bank,offset,uint64_t(pages)*4096)
#endif
			;
	}
	bool Page(const tt_dram_samples_result& baseline,unsigned bank,uint64_t offset,
		uint32_t& address,uint32_t& route,uint32_t& index) const {
		if (!Range(baseline,bank,offset)) return false;
		address=Base+uint32_t(offset); route=routes[bank];
		index=uint32_t(uint64_t(bank)*(Capacity/4096)+offset/4096); return true;
	}
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	// A larger learned extent must prove ALL bytes disjoint, not just its
	// first page. This does not widen the legacy <=4-page Range contract.
	bool ImmutableRange(const tt_dram_samples_result& baseline,unsigned bank,uint64_t offset,uint64_t bytes) const {
		return bank<7 && bytes && offset<Capacity && bytes<=Capacity-offset && Matches(baseline) && !MutableOverlap(bank,offset,bytes);
	}
	// Caller holds the global store-owner lock and freezes the private ranges
	// and initialized bitmap. Validate ALL pages BEFORE publishing ANY claim.
	// No bitmap tag is reused: state never masquerades as uploaded weights.
	bool ReserveMutable(const tt_dram_samples_result& baseline,const void* owner,
		const PioneerResidentMutableRange* ranges,unsigned count,const uint8_t* initialized,unsigned initializedPages) {
		if (!owner || mutableOwner || !ranges || !count || count>673 || !initialized || initializedPages!=Pages || !Matches(baseline)) return false;
		for (unsigned i=0;i<count;i++) {
			const auto& r=ranges[i];
			if (r.bank>=7 || !r.pages || r.firstPage>=Capacity/4096 || r.pages>Capacity/4096-r.firstPage) return false;
			for (unsigned j=0;j<i;j++) {
				const auto& a=ranges[j];
				if (r.bank==a.bank && r.firstPage<a.firstPage+a.pages && a.firstPage<r.firstPage+r.pages) return false;
			}
			for (unsigned p=0;p<r.pages;p++) {
				uint32_t address=0,route=0,index=0;
				if (!Page(baseline,r.bank,uint64_t(r.firstPage+p)*4096,address,route,index)
					|| index>=initializedPages || initialized[index]) return false;
			}
		}
		for (unsigned i=0;i<count;i++) {
			auto r=ranges[i]; unsigned at=i;
			while (at && (mutableRanges[at-1].bank>r.bank
				|| (mutableRanges[at-1].bank==r.bank && mutableRanges[at-1].firstPage>r.firstPage))) {
				mutableRanges[at]=mutableRanges[at-1]; at--;
			}
			mutableRanges[at]=r;
		}
		mutableCount=count; mutableOwner=owner; return true;
	}
	// Reservation is sticky until the entire bank/store context is destroyed.
	// No release/reset API. This only derives a private reserved page; it
	// does NOT prove initialization, completion, or launch permission.
	bool MutablePage(const tt_dram_samples_result& baseline,const void* owner,unsigned bank,uint64_t offset,
		uint32_t& address,uint32_t& route,uint32_t& index) const {
		if (!owner || owner!=mutableOwner || bank>=7 || (offset&4095) || offset>=Capacity || !Matches(baseline)) return false;
		if (!MutableOverlap(bank,offset,4096)) return false;
		address=Base+uint32_t(offset); route=routes[bank]; index=uint32_t(uint64_t(bank)*(Capacity/4096)+offset/4096); return true;
	}
#endif
};
#endif
