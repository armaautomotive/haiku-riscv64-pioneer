/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_PUBLISHER_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_PUBLISHER_H

#include "PioneerQwen35DataflowGraphRunProtocol.h"
#include "PioneerQwen35DataflowOwner.h"
#include "PioneerQwen35DataflowStreamPlan.h"

namespace PioneerQwen35 {
enum DataflowGraphPublication { DataflowGraphPublishInvalid=0,DataflowGraphPublishPreflight=1,
	DataflowGraphPublishStagingFault=2,DataflowGraphPublishCommandFault=3,
	DataflowGraphPublished=4 };

// Called under the exclusive model/session lock with a kernel-validated,
// sealed plan, model-bound token context and fresh ticket. All workers are
// checked, then all descriptors and command markers are staged/read back,
// before any trigger is published. A failed stage or partial publication
// retains the complete graph; this class never asserts completion or frees it.
class DataflowGraphPublisher {
	enum Phase { PhaseEmpty,PhasePublished,PhaseFaulted } phase=PhaseEmpty;
	unsigned failedCore=DataflowCoreCount;
public:
	DataflowGraphPublisher()=default;
	DataflowGraphPublisher(const DataflowGraphPublisher&)=delete;
	DataflowGraphPublisher& operator=(const DataflowGraphPublisher&)=delete;
	bool Faulted() const { return phase==PhaseFaulted; }
	unsigned FailedCore() const { return failedCore; }
	template<class IO>
	DataflowGraphPublication Start(const DataflowStreamPlan& plan,
		const unsigned (&routes)[2][Banks],const DataflowDynamicTokenContext& token,
		const DataflowGraphRunTicket& ticket,unsigned trigger,unsigned pollLimit,
		IO& io,DataflowGraphRunDescriptor& scratch)
	{
		if (phase!=PhaseEmpty || !plan.ready || !plan.epoch || !plan.count
			|| plan.count!=ticket.count || !token.Valid() || !trigger
			|| trigger>DataflowWorkerSequenceLimit || !pollLimit || pollLimit>DataflowGraphRunMaxPolls
			|| !DataflowBankRoutesValid(routes)) return DataflowGraphPublishInvalid;
		unsigned completed=0;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowWorkerSnapshot snapshot{};
			if (!io.ReadWorkerSnapshot(core,snapshot) || !DataflowWorkerSnapshotReady(snapshot)
				|| !io.CoreReady(core,snapshot) || (core && snapshot.completedSequence!=completed)
				|| trigger<=snapshot.completedSequence) {
				failedCore=core;return DataflowGraphPublishPreflight;
			}
			completed=snapshot.completedSequence;
			if (plan.binding[core].epoch!=plan.epoch || plan.binding[core].count!=plan.count
				|| !DataflowGraphRunBuild(plan.binding[core],plan.lease[core],routes,
					token,ticket,trigger,pollLimit,scratch)) {
				failedCore=core;return DataflowGraphPublishPreflight;
			}
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!DataflowGraphRunBuild(plan.binding[core],plan.lease[core],routes,
					token,ticket,trigger,pollLimit,scratch)
				|| !io.StageGraphRun(core,scratch) || !io.GraphRunMatches(core,scratch)) {
				phase=PhaseFaulted;failedCore=core;return DataflowGraphPublishStagingFault;
			}
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!io.PublishGraphCommand(core,trigger)) {
				phase=PhaseFaulted;failedCore=core;return DataflowGraphPublishCommandFault;
			}
		}
		phase=PhasePublished;return DataflowGraphPublished;
	}
};
}
#endif
