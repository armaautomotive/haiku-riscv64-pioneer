/* Distributed under the MIT License. */
#ifndef SG2042_RECOVERY_LEASE_H
#define SG2042_RECOVERY_LEASE_H

#include <stdint.h>

// Thread-scoped config-space exclusion for the private Pioneer root. The
// containing controller serializes every method with the same config mutex.
// This does not exclude BAR MMIO, DMA, firmware, or access outside the Haiku
// PCI controller. It never changes hardware and grants no reset authority.
class Sg2042RecoveryLease {
	uint64_t generation=0,active=0;
	int64_t owner=-1;
public:
	bool Begin(bool eligible,int64_t thread,uint64_t& ticket) {
		ticket=0;
		if (!eligible || thread<0 || active || generation==UINT64_MAX) return false;
		active=++generation;owner=thread;ticket=active;return true;
	}
	bool Allows(int64_t thread) const { return !active || (thread>=0 && thread==owner); }
	bool End(int64_t thread,uint64_t ticket) {
		if (!active || !ticket || ticket!=active || thread<0 || thread!=owner) return false;
		active=0;owner=-1;return true;
	}
	bool Active() const { return active!=0; }
};
#endif
