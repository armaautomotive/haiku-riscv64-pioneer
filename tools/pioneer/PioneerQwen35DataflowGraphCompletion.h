/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_COMPLETION_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_COMPLETION_H

#include "PioneerQwen35DataflowGraphPublisher.h"

namespace PioneerQwen35 {
enum DataflowGraphCompletionResult { DataflowGraphCompletionInvalid=0,
	DataflowGraphCompletionOwnershipFault=1,DataflowGraphCompletionWorkerFault=2,
	DataflowGraphCompletionAuditFault=3,DataflowGraphCompletionResultFault=4,
	DataflowGraphCompletionAcknowledgeFault=5,DataflowGraphCompletionReadyFault=6,
	DataflowGraphCompletionAudited=7 };

// This audits one already-published graph and acknowledges only after every
// worker has passed. A partial acknowledgement or uncertain completion keeps
// the graph owner and all allocations retained. An audited result still does
// not advance the live model authority; the session owner must do that once.
class DataflowGraphCompletion {
	enum Phase { Empty,Audited,Faulted } phase=Empty;
	unsigned failedCore=DataflowCoreCount;
	DataflowWorkerSnapshot receipts[DataflowCoreCount]{};
	DataflowGraphCompletionResult Fail(DataflowGraphCompletionResult r,unsigned core) {
		phase=Faulted;failedCore=core;return r;
	}
public:
	DataflowGraphCompletion()=default;
	DataflowGraphCompletion(const DataflowGraphCompletion&)=delete;
	DataflowGraphCompletion& operator=(const DataflowGraphCompletion&)=delete;
	bool FaultedState() const { return phase==Faulted; }
	unsigned FailedCore() const { return failedCore; }
	template<class IO>
	DataflowGraphCompletionResult Finish(const DataflowGraphPublisher& publisher,
		const DataflowStreamPlan& plan,const unsigned (&routes)[2][Banks],
		const DataflowDynamicTokenContext& token,IO& io,DataflowGraphRunDescriptor& scratch,
		DataflowArgmaxResult& output)
	{
		output={Vocabulary,0,0,0};
		if (phase!=Empty || !publisher.Published() || !plan.ready
			|| (plan.count!=DataflowGraphDecodeSteps && plan.count!=DataflowGraphPromptSteps)
			|| (publisher.Ticket().count!=DataflowGraphDecodeSteps
				&& publisher.Ticket().count!=DataflowGraphPromptSteps)
			|| publisher.Ticket().count>plan.count) return DataflowGraphCompletionInvalid;
		if (!io.GraphOwnershipMatches(plan,token,publisher.Ticket(),publisher.InputSlot()))
			return Fail(DataflowGraphCompletionOwnershipFault,DataflowCoreCount);
		const unsigned trigger=publisher.TriggerSequence(),previous=publisher.PreviousSequence();
		const bool decode=publisher.Ticket().count==DataflowGraphDecodeSteps;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			auto& r=receipts[core];
			if (!io.WaitForCompletion(core,trigger,r) || r.commandSequence!=trigger
				|| r.acknowledgedSequence!=previous || r.completedSequence!=trigger
				|| r.state!=DataflowWorkerComplete || r.failure || r.phase!=2)
				return Fail(DataflowGraphCompletionWorkerFault,core);
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!DataflowGraphRunBuild(plan.binding[core],plan.lease[core],routes,token,
					publisher.Ticket(),trigger,publisher.PollLimit(),publisher.InputSlot(),scratch)
				|| !io.AuditGraphCore(core,scratch,receipts[core],decode))
				return Fail(DataflowGraphCompletionAuditFault,core);
		}
		DataflowArgmaxResult candidate{};
		if (decode) {
			DataflowArgmaxResult again{};
			const unsigned sequence=publisher.Ticket().firstSequence+publisher.Ticket().count-1;
			if (!io.ReadArgmaxResult(sequence,candidate) || !io.ReadArgmaxResult(sequence,again)
				|| !DataflowArgmaxResultValid(candidate) || candidate.sequence!=sequence
				|| candidate.token!=again.token || candidate.scoreBits!=again.scoreBits
				|| candidate.sequence!=again.sequence || candidate.checksum!=again.checksum)
				return Fail(DataflowGraphCompletionResultFault,0);
		}
		for (unsigned core=0;core<DataflowCoreCount;core++)
			if (!io.Acknowledge(core,trigger))
				return Fail(DataflowGraphCompletionAcknowledgeFault,core);
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			bool ready=false;
			for (unsigned poll=0;poll<publisher.PollLimit();poll++) {
				DataflowWorkerSnapshot snapshot{};
				if (io.ReadWorkerSnapshot(core,snapshot) && DataflowWorkerSnapshotReady(snapshot)
					&& snapshot.completedSequence==trigger && io.CoreReady(core,snapshot)) {
					ready=true;break;
				}
			}
			if (!ready) return Fail(DataflowGraphCompletionReadyFault,core);
		}
		phase=Audited;if (decode) output=candidate;return DataflowGraphCompletionAudited;
	}
};
}
#endif
