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
class PioneerResidentBanks {
	const tt_dram_samples_result* bound = nullptr;
	uint32_t routes[TT_DRAM_BANKS] = {};
	uint32_t source[TT_DRAM_SOURCE_WORDS] = {};
	bool quarantined = false;
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
			&& uint64_t(pages)*4096<=Capacity-offset && Matches(baseline);
	}
	bool Page(const tt_dram_samples_result& baseline,unsigned bank,uint64_t offset,
		uint32_t& address,uint32_t& route,uint32_t& index) const {
		if (!Range(baseline,bank,offset)) return false;
		address=Base+uint32_t(offset); route=routes[bank];
		index=uint32_t(uint64_t(bank)*(Capacity/4096)+offset/4096); return true;
	}
};
#endif
