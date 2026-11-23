/* Distributed under the MIT License. */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>
#include "../../../src/add-ons/kernel/busses/pci/ecam/Sg2042RecoveryLease.h"

int main()
{
	Sg2042RecoveryLease lease,other;
	uint64_t ticket=99,second=99;
	assert(lease.Allows(-1) && lease.Allows(1) && !lease.Active());
	assert(!lease.Begin(false,1,ticket) && !ticket && !lease.Active());
	assert(!lease.Begin(true,-1,ticket) && !ticket && !lease.Active());
	assert(lease.Begin(true,11,ticket) && ticket==1 && lease.Active());
	assert(lease.Allows(11) && !lease.Allows(12) && !lease.Allows(-1));
	assert(other.Allows(12) && other.Begin(true,12,second) && second==1);
	assert(!lease.Begin(true,11,second) && !second);
	assert(!lease.End(12,ticket) && !lease.End(11,ticket+1) && lease.Active());
	assert(lease.End(11,ticket) && !lease.Active() && lease.Allows(12));
	assert(!lease.End(11,ticket));
	assert(lease.Begin(true,12,second) && second==2);
	assert(!lease.End(12,ticket) && lease.End(12,second));
	assert(other.End(12,1));

	// The controller's config mutex surrounds each operation; contend through
	// the same lock and verify that only one thread obtains a lease.
	Sg2042RecoveryLease shared;std::mutex lock;unsigned wins=0;uint64_t retained=0;
	std::thread threads[16];
	for (int id=0;id<16;id++) threads[id]=std::thread([&,id] {
		std::lock_guard<std::mutex> guard(lock);uint64_t candidate=0;
		if (shared.Begin(true,id,candidate)) { ++wins;retained=candidate; }
	});
	for (auto& thread:threads) thread.join();
	assert(wins==1 && retained==1 && shared.Active());
	unsigned allowed=0;int owner=-1;
	for (int id=0;id<16;id++) if (shared.Allows(id)) {++allowed;owner=id;}
	assert(allowed==1 && shared.End(owner,retained) && !shared.Active());
	std::puts("PASS private SG2042 PCI config lease: eligibility, owner, stale ticket and contention; SIMULATED ONLY");
}
