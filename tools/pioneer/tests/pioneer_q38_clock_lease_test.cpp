/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_ARC_QUEUE_MODEL_ONLY
#include "pioneer_q38_arc_queue_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ClockLease.h"

struct ClockLeaseIO:StrictQueue {
	unsigned maps=0,unmaps=0;
	bool failMap=false,failUnmap=false;
	bool Map() { maps++; return !failMap; } // virtual views ONLY
	bool Unmap() { unmaps++; return !failUnmap; }
};
#ifndef PIONEER_Q38_CLOCK_LEASE_MODEL_ONLY
int main()
{
	const auto arc=StrictQueue::Arc(); const auto health=StrictQueue::Health();
	{
		ClockLeaseIO io; PioneerQ38ClockLease<ClockLeaseIO> lease(io);
		assert(lease.End(arc,health,true) && !io.io && !io.maps && !io.unmaps);
		assert(lease.Begin(arc,health) && lease.RequiresExplicitEnd() && io.maps==1 && io.clock==1350);
		const unsigned before=io.io; assert(!lease.Begin(arc,health) && io.io==before && io.maps==1);
		assert(lease.End(arc,health,true) && !lease.RequiresExplicitEnd() && io.unmaps==1 && io.clock==800 && io.Restored());
		const unsigned done=io.io; assert(lease.End(arc,health,true) && io.io==done && io.unmaps==1);
	}
	for (unsigned fault=0;fault<5;fault++) {
		ClockLeaseIO io; PioneerQ38ClockLease<ClockLeaseIO> lease(io);
		assert(lease.Begin(arc,health));
		if (!fault) lease.Quarantine(); else if (fault==1) io.timeout=true;
		else if (fault==2) io.badReply=true; else if (fault==3) io.failRestore=true;
		const unsigned before=io.io;
		assert(!lease.End(arc,health,fault!=4) && lease.Unsafe() && lease.RequiresExplicitEnd() && !io.unmaps);
		if (!fault || fault==4) assert(io.io==before); // unknown compute/held proof
		const unsigned done=io.io;
		assert(!lease.End(arc,health,true) && !lease.Begin(arc,health) && io.io==done && !io.unmaps && io.maps==1);
	}
	{
		ClockLeaseIO io; io.failMap=true; PioneerQ38ClockLease<ClockLeaseIO> lease(io);
		assert(!lease.Begin(arc,health) && !io.io && !lease.Unsafe() && lease.RequiresExplicitEnd());
		assert(lease.End(arc,health,true) && io.unmaps==1 && !io.io);
	}
	{
		ClockLeaseIO io; io.failUnmap=true; PioneerQ38ClockLease<ClockLeaseIO> lease(io);
		assert(lease.Begin(arc,health) && !lease.End(arc,health,true) && lease.Unsafe() && io.clock==800 && io.unmaps==1);
		const unsigned done=io.io; assert(!lease.End(arc,health,true) && io.io==done && io.unmaps==1);
	}
	{
		ClockLeaseIO io;
		{ PioneerQ38ClockLease<ClockLeaseIO> lease(io); assert(lease.Begin(arc,health)); }
		assert(io.clock==1350 && !io.unmaps && io.notifications==3); // destructor cannot clean up
	}
	puts("PASS OFFLINE clock lease:normal explicit End,partial-map disposal ONLY when known-clean,compute/transport/hold/restore faults retain views and forbid ALL later IO/retry/idle/unmap;destructor has NO cleanup");
}
#endif
