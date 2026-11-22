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
	unsigned previousSequence=0,triggerSequence=0,polls=0,inputSlot=0;
	DataflowGraphRunTicket runTicket{};
public:
	DataflowGraphPublisher()=default;
	DataflowGraphPublisher(const DataflowGraphPublisher&)=delete;
	DataflowGraphPublisher& operator=(const DataflowGraphPublisher&)=delete;
	bool Faulted() const { return phase==PhaseFaulted; }
	bool Published() const { return phase==PhasePublished; }
	unsigned FailedCore() const { return failedCore; }
	unsigned PreviousSequence() const { return previousSequence; }
	unsigned TriggerSequence() const { return triggerSequence; }
	unsigned PollLimit() const { return polls; }
	unsigned InputSlot() const { return inputSlot; }
	const DataflowGraphRunTicket& Ticket() const { return runTicket; }
	template<class IO>
	DataflowGraphPublication Start(const DataflowStreamPlan& plan,
		const unsigned (&routes)[2][Banks],const DataflowDynamicTokenContext& token,
		const DataflowGraphRunTicket& ticket,unsigned trigger,unsigned pollLimit,unsigned slot,
		IO& io,DataflowGraphRunDescriptor& scratch)
	{
		if (phase!=PhaseEmpty || !plan.ready || !plan.epoch
			|| (plan.count!=DataflowGraphPromptSteps && plan.count!=DataflowGraphDecodeSteps)
			|| (ticket.count!=DataflowGraphPromptSteps && ticket.count!=DataflowGraphDecodeSteps)
			|| ticket.count>plan.count || !token.Valid() || !trigger
			|| trigger>DataflowWorkerSequenceLimit || !pollLimit || pollLimit>DataflowGraphRunMaxPolls || slot>=ActivationBuffers
			|| !DataflowBankRoutesValid(routes)) return DataflowGraphPublishInvalid;
		// Structural validity is not authority: the owner must bind the
		// sealed template (including its fixed activation slot), model, token
		// and fresh ticket before any worker memory is written.
		if (!io.GraphOwnershipMatches(plan,token,ticket,slot)) return DataflowGraphPublishPreflight;
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
					token,ticket,trigger,pollLimit,slot,scratch)) {
				failedCore=core;return DataflowGraphPublishPreflight;
			}
		}
		previousSequence=completed;triggerSequence=trigger;polls=pollLimit;inputSlot=slot;runTicket=ticket;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!DataflowGraphRunBuild(plan.binding[core],plan.lease[core],routes,
					token,ticket,trigger,pollLimit,slot,scratch)
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
