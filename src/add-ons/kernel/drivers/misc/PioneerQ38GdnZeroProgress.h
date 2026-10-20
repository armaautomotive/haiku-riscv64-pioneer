/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_ZERO_PROGRESS_H
#define PIONEER_Q38_GDN_ZERO_PROGRESS_H
#include "PioneerQ38GdnIdentity.h"
// CPU metadata ONLY. Never initializes memory or grants a compute/read
// capability. The private bank owner calls Record only after its complete
// device-zero proof;the public caller cannot access that embedded ledger.
class PioneerQ38GdnZeroProgress {
	PioneerQ38GdnIdentity identity={};
	static constexpr unsigned PageCount=48*2*48*16;
	uint32_t bits[PageCount/32]={}; // owner allocated on HEAP,not kernel stack
	unsigned completed=0;
	bool bound=false,unsafe=false;
public:
	static constexpr unsigned Pages=PageCount;
	bool Bind(const PioneerQ38GdnIdentity& id) {
		if (bound || unsafe || !id.Valid()) return false;
		identity=id; bound=true; return true;
	}
	bool Record(const PioneerQ38GdnIdentity& id,unsigned first,unsigned pages) {
		if (!bound || unsafe || !(id==identity) || !pages || pages>256 || first>=Pages || pages>Pages-first) return false;
		for (unsigned at=first;at<first+pages;at++) {
			uint32_t mask=UINT32_C(1)<<(at%32);
			if (!(bits[at/32]&mask)) { bits[at/32]|=mask; completed++; }
		}
		return true; // overlaps/repeated zeros NEVER increase progress twice
	}
	bool Snapshot(const PioneerQ38GdnIdentity& id,unsigned& pages) const {
		if (!bound || unsafe || !(id==identity)) return false;
		pages=completed; return true; // count ONLY,not whole-state/model ready
	}
	void Fault() { unsafe=true; } // sticky,NO reset/rebind/rollback/release
};
#endif
