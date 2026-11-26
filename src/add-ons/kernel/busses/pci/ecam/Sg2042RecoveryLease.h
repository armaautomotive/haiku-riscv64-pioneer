/* Distributed under the MIT License. */
#ifndef SG2042_RECOVERY_LEASE_H
#define SG2042_RECOVERY_LEASE_H

#include <stdint.h>
#if defined(PIONEER_SG2042_RETAINED_LEASE_WITNESS) \
	&& (!defined(PIONEER_SG2042_RECOVERY_WITNESS) || defined(PIONEER_SG2042_RECOVERY_QUALIFICATION) \
		|| defined(PIONEER_SG2042_RECOVERY_GATEWAY_WITNESS))
#error Retained lease is restricted to an isolated non-loadable controller witness
#endif

// Thread-scoped config-space exclusion for the private Pioneer root. The
// containing controller serializes every method with the same config mutex.
// This does not exclude BAR MMIO, DMA, firmware, or access outside the Haiku
// PCI controller. It never changes hardware and grants no reset authority.
class Sg2042RecoveryLease {
	uint64_t generation=0,active=0;
	int64_t owner=-1;
#ifdef PIONEER_SG2042_RETAINED_LEASE_WITNESS
	const void* retainedOwner=nullptr;
	uint64_t retainedSession=0;
	bool parked=false;
	bool Matches(const void* identity,uint64_t session,uint64_t ticket) const {
		return active && ticket==active && identity && identity==retainedOwner
			&& session && session==retainedSession;
	}
#endif
public:
	bool Begin(bool eligible,int64_t thread,uint64_t& ticket) {
		ticket=0;
		if (!eligible || thread<0 || active || generation==UINT64_MAX) return false;
		active=++generation;owner=thread;ticket=active;return true;
	}
	bool Allows(int64_t thread) const { return !active || (thread>=0 && thread==owner); }
	bool End(int64_t thread,uint64_t ticket) {
	#ifdef PIONEER_SG2042_RETAINED_LEASE_WITNESS
		if (retainedOwner) return false;
	#endif
		if (!active || !ticket || ticket!=active || thread<0 || thread!=owner) return false;
		active=0;owner=-1;return true;
	}
	bool Active() const { return active!=0; }
#ifdef PIONEER_SG2042_RETAINED_LEASE_WITNESS
	// The identity and session are kernel-owned. Their lifetime, uniqueness,
	// device pinning and recovery proof are obligations of the future caller.
	// This witness supplies exclusion only and is not a hardware reset API.
	bool BeginRetained(bool eligible,int64_t thread,const void* identity,uint64_t session,uint64_t& ticket) {
		ticket=0;
		if (!identity || !session || !Begin(eligible,thread,ticket)) return false;
		retainedOwner=identity;retainedSession=session;parked=false;return true;
	}
	bool Park(int64_t thread,const void* identity,uint64_t session,uint64_t ticket) {
		if (parked || thread<0 || thread!=owner || !Matches(identity,session,ticket)) return false;
		owner=-1;parked=true;return true;
	}
	bool Resume(bool eligible,int64_t thread,const void* identity,uint64_t session,uint64_t ticket) {
		if (!eligible || !parked || thread<0 || !Matches(identity,session,ticket)) return false;
		owner=thread;parked=false;return true;
	}
	bool Finish(int64_t thread,const void* identity,uint64_t session,uint64_t ticket,bool releaseAuthorized) {
		if (!releaseAuthorized || parked || thread<0 || thread!=owner || !Matches(identity,session,ticket)) return false;
		active=0;owner=-1;retainedOwner=nullptr;retainedSession=0;return true;
	}
	bool Parked() const { return active && parked; }
#endif
};
#endif
