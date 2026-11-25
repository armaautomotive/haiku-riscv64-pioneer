/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BARRIER_SETUP_H
#define PIONEER_QWEN35_DATAFLOW_BARRIER_SETUP_H

#include "PioneerQwen35DataflowBarrierLayout.h"
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
#include "PioneerQwen35DataflowActivationShareLayout.h"
#endif

namespace PioneerQwen35 {

// Install while every core is held and after its original L1 was saved. The
// caller must retain all snapshots and refuse to start any worker if a write,
// readback, or hold check fails; an uncertain write is never retried here.
template<class IO>
bool DataflowBarrierPrepareHeldCore(IO& io,unsigned core)
{
	if (core>=DataflowBarrierWorkers || !io.CoreHeld(core)) return false;
	uint32_t zero[DataflowBarrierPacketBytes/sizeof(uint32_t)]={};
	uint32_t observed[DataflowBarrierPacketBytes/sizeof(uint32_t)];
	auto clear=[&](unsigned address) {
		if (!io.CoreHeld(core) || !io.WriteCoreMemory(core,address,zero,sizeof(zero))
			|| !io.ReadCoreMemory(core,address,observed,sizeof(observed))) return false;
		for (unsigned i=0;i<sizeof(observed)/sizeof(observed[0]);i++) if (observed[i]) return false;
		return io.CoreHeld(core);
	};
	if (!clear(DataflowBarrierOutbox) || !clear(DataflowBarrierReleasePayload)
		|| !clear(DataflowBarrierReleaseCommit)) return false;
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
	// No run may reuse an epoch/step until the old ready record is removed.
	// All workers stay held until every core's clear and readback succeeds.
	if (!clear(DataflowActivationShareReadyAddress)
		|| !clear(DataflowActivationShareScratchAddress)) return false;
#endif
	const unsigned first=core*DataflowBarrierReleaseFanout+1;
	for (unsigned worker=first;worker<first+DataflowBarrierReleaseFanout
		&& worker<DataflowBarrierWorkers;worker++)
		if (!clear(DataflowBarrierArrivalAddress(worker,false))
			|| !clear(DataflowBarrierArrivalAddress(worker,true))) return false;
	return true;
}
}
#endif
