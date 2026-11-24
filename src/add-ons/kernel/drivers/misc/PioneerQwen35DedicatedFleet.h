/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DEDICATED_FLEET_H
#define PIONEER_QWEN35_DEDICATED_FLEET_H

// Prepare only the per-worker NoC/store leases needed by the Qwen35 resident
// service. All workers stay held: this path never writes or boots Q38 images.
// A partial or uncertain lease operation quarantines the complete fleet.
template<class IO,class Dram,class Banks,class Lease>
bool PioneerQwen35DedicatedFleetPrepare(IO& io,const Dram& dram,Banks& banks,Lease& lease)
{
	if (lease.unsafe || lease.closed || lease.rootRunning || lease.booted
		|| lease.bootAttempted || lease.prepared) return false;
	for (unsigned core=0;core<120;core++) {
		auto& node=lease.worker[core];
		lease.prepared=core+1; // includes any partially touched lease
		if (!io.SelectRuntimeCore(core) || !io.MapQ38Panel()
			|| (core && !PioneerQwen35DedicatedPeerLeaseBegin(io,dram,core,node))
			|| node.image.saved || node.image.ready || node.image.running || node.image.unsafe
			|| !PioneerQ38DispatchSources(io,dram,banks,node,0x47800))
			return PioneerQ38DispatchQuarantine(lease,banks);
	}
	// Preparation ends on worker 119. Scratch transfers and the projection
	// scope begin on worker zero; leave that route selected and re-audit its
	// held sources. A failed route change may be partial, so retain the fleet.
	if (!io.SelectRuntimeCore(0)
		|| !PioneerQ38DispatchSources(io,dram,banks,lease.worker[0],0x47800))
		return PioneerQ38DispatchQuarantine(lease,banks);
	return true;
}

// Once the snapshot-based service has restored every role and resident
// image, its closed owner may still exist until the session is destroyed.
// The role binding and its mapping owner must already have been released.
template<class Service,class Memory,class Binding>
bool PioneerQwen35DedicatedServiceReleased(const Service* service,
    const Memory* memory,const Binding* binding)
{
	return (!service || service->Closed()) && !memory && !binding;
}

// Used after the service has restored its snapshots, or before it has touched
// a worker. Validate every held tile before restoring any peer lease.
template<class IO,class Dram,class Banks,class Lease>
bool PioneerQwen35DedicatedFleetClose(IO& io,const Dram& dram,Banks& banks,Lease& lease)
{
	if (lease.unsafe || lease.closed || lease.rootRunning || lease.booted
		|| lease.bootAttempted || lease.prepared!=120) return false;
	for (unsigned core=0;core<120;core++) {
		auto& node=lease.worker[core];
		if (node.image.saved || node.image.ready || node.image.running || node.image.unsafe
			|| !io.SelectRuntimeCore(core) || !PioneerQ38DispatchSources(io,dram,banks,node,0x47800))
			return PioneerQ38DispatchQuarantine(lease,banks);
	}
	for (unsigned end=120;end>1;end--) {
		auto& node=lease.worker[end-1];
		if (!io.SelectRuntimeCore(end-1) || !PioneerQwen35DedicatedPeerLeaseEnd(io,node.noc,node.Store()))
			return PioneerQ38DispatchQuarantine(lease,banks);
	}
	if (!io.SelectRuntimeCore(0)) return PioneerQ38DispatchQuarantine(lease,banks);
	lease.closed=true;return true;
}
#endif
