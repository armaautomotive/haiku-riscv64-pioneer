/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BARRIER_LAYOUT_H
#define PIONEER_QWEN35_DATAFLOW_BARRIER_LAYOUT_H

#include <stdint.h>
#include "PioneerQwen35DataflowNocProtocol.h"

namespace PioneerQwen35 {

// All ranges are in each worker's saved 0x6e000..0xb0000 L1 view. The
// coordinator is also worker 0 and still participates in matrix operations.
// Each peer has a unique two-packet arrival slot at the coordinator; the
// separate release inbox is private to its destination worker.
enum : unsigned {
	DataflowBarrierWorkers=120,
	DataflowBarrierReleaseFanout=4,
	DataflowBarrierPacketBytes=64,
	DataflowBarrierOutbox=0xab040,
	DataflowBarrierReleasePayload=0xab080,
	DataflowBarrierReleaseCommit=0xab0c0,
	DataflowBarrierArrivalBase=0xac000,
	DataflowBarrierArrivalStride=2*DataflowBarrierPacketBytes,
	DataflowBarrierArrivalEnd=DataflowBarrierArrivalBase
		+DataflowBarrierWorkers*DataflowBarrierArrivalStride,
	DataflowBarrierL1Limit=0xb0000
};
static_assert(DataflowBarrierOutbox+DataflowBarrierPacketBytes==DataflowBarrierReleasePayload
	&& DataflowBarrierReleasePayload+DataflowBarrierPacketBytes==DataflowBarrierReleaseCommit
	&& DataflowBarrierReleaseCommit+DataflowBarrierPacketBytes<=DataflowBarrierArrivalBase
	&& DataflowBarrierArrivalEnd<=DataflowBarrierL1Limit,
	"barrier outbox, release inbox and per-core arrivals do not overlap");

inline unsigned DataflowBarrierArrivalAddress(unsigned worker,bool commit)
{
	return worker<DataflowBarrierWorkers
		?DataflowBarrierArrivalBase+worker*DataflowBarrierArrivalStride
			+(commit?unsigned(DataflowBarrierPacketBytes):0U):UINT32_MAX;
}

inline unsigned DataflowBarrierWorkerRoute(unsigned worker)
{
	const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
	return worker<DataflowBarrierWorkers
		?columns[worker/10]|((2+worker%10)<<6):UINT32_MAX;
}

// A fixed four-way heap tree distributes a completed coordinator decision.
// Every non-root worker has exactly one parent with a smaller index. Only
// that parent may write its release inbox; arrivals still go directly to 0.
inline unsigned DataflowBarrierReleaseParent(unsigned worker)
{
	return worker && worker<DataflowBarrierWorkers
		?(worker-1)/DataflowBarrierReleaseFanout:UINT32_MAX;
}

inline bool DataflowBarrierReleaseEdge(unsigned source,unsigned destination)
{
	return source<DataflowBarrierWorkers && destination<DataflowBarrierWorkers
		&& DataflowBarrierReleaseParent(destination)==source;
}

// No arbitrary address, core or route is supplied to the NoC transport.
// A peer may publish only its own coordinator slot; a release may travel only
// along the fixed tree. The core-0 local receipt needs no NoC packet.
inline bool DataflowBarrierPeerWriteAllowed(unsigned source,unsigned destination,
	unsigned route,unsigned remote,unsigned local,unsigned bytes)
{
	if (source>=DataflowBarrierWorkers || destination>=DataflowBarrierWorkers
		|| source==destination || route!=DataflowBarrierWorkerRoute(destination)
		|| local!=DataflowBarrierOutbox || bytes!=DataflowBarrierPacketBytes) return false;
	if (destination==0 && source!=0)
		return remote==DataflowBarrierArrivalAddress(source,false)
			|| remote==DataflowBarrierArrivalAddress(source,true);
	if (DataflowBarrierReleaseEdge(source,destination))
		return remote==DataflowBarrierReleasePayload || remote==DataflowBarrierReleaseCommit;
	return false;
}
}
#endif
