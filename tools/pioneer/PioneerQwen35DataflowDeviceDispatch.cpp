/* SPDX-License-Identifier: MIT */
#include "PioneerQwen35DataflowDeviceGraph.h"
#include "PioneerQwen35DataflowDynamicToken.h"
#include "PioneerQwen35DataflowGraphRunProtocol.h"
#include "PioneerQwen35DataflowGraphAuthorization.h"

// The graph dispatcher remains non-loadable until the kernel owns its sealed
// model and session lifetimes and the device path has been qualified.
extern "C" __attribute__((noinline)) bool qwen35_graph_authorize(
	const PioneerQwen35::DataflowGraphRunDescriptor* run,unsigned index) {
	using namespace PioneerQwen35;
	if (!run) return false;
	const auto* record=reinterpret_cast<const DataflowStreamRecord*>(static_cast<uintptr_t>(DataflowStreamStagingAddress));
	const auto* program=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(DataflowProgramAddress));
	return DataflowGraphFetchedCommandMatches(*run,index,*record,program,DataflowProgramRegionBytes);
}
extern "C" bool qwen35_graph_execute(unsigned core,unsigned sequence,
	PioneerQwen35::DataflowWorkerMailbox* mailbox);
extern "C" bool qwen35_graph_reset_queues();

struct GraphWitnessExecution {
	const PioneerQwen35::DataflowGraphRunDescriptor& run;
	PioneerQwen35::DataflowWorkerMailbox* mailbox;
	bool Fetch(unsigned index) {
		return PioneerQwen35::DataflowBlackholeStreamFetchDynamic(run.binding,run.lease,run.routes,run.core,index,run.token);
	}
	bool Authorize(unsigned index) { return qwen35_graph_authorize(&run,index); }
	bool Execute(unsigned sequence,unsigned& status,unsigned& detail) {
		status=detail=0;return qwen35_graph_execute(run.core,sequence,mailbox);
	}
	bool ResetQueues() { return qwen35_graph_reset_queues(); }
};
extern "C" unsigned PioneerQwen35DeviceGraphRun(
	const PioneerQwen35::DataflowGraphRunDescriptor& run,
	PioneerQwen35::DataflowWorkerMailbox* mailbox)
{
	if (!mailbox || !PioneerQwen35::DataflowGraphRunValid(run,mailbox->logicalCore)) return PioneerQwen35::DataflowGraphInvalid;
	GraphWitnessExecution execution{run,mailbox};
	PioneerQwen35::DataflowBlackholeBarrierIO transport(run.core);
	PioneerQwen35::DataflowDeviceGraph<GraphWitnessExecution,
		PioneerQwen35::DataflowBlackholeBarrierIO> graph(execution,transport);
	if (!graph.Start(run.binding,run.core,run.pollLimit,run.ticket.epoch,run.ticket.firstSequence,run.ticket.count))
		return PioneerQwen35::DataflowGraphInvalid;
	for (;;) {
		const auto state=graph.Tick();
		if (state==PioneerQwen35::DataflowGraphRetain) {
			using namespace PioneerQwen35;
			DataflowStoreRelease(mailbox->reserved[1],graph.Fault());
			DataflowStoreRelease(mailbox->reserved[2],graph.CompletedSteps());
			DataflowStoreRelease(mailbox->reserved[3],graph.FailureCore());
			DataflowStoreRelease(mailbox->reserved[4],graph.FailureStatus());
			DataflowFence();
			DataflowStoreRelease(mailbox->reserved[0],DataflowGraphFaultMagic);
		}
		if (state==PioneerQwen35::DataflowGraphLocalComplete || state==PioneerQwen35::DataflowGraphRetain)
			return state;
	}
}

extern "C" bool qwen35_graph_run_descriptor(
	const PioneerQwen35::DataflowGraphRunDescriptor* descriptor,
	PioneerQwen35::DataflowWorkerMailbox* mailbox)
{
	if (!descriptor || !mailbox || !PioneerQwen35::DataflowGraphRunValid(*descriptor,mailbox->logicalCore)
		|| descriptor->triggerSequence!=PioneerQwen35::DataflowLoadAcquire(mailbox->commandSequence)) return false;
	return PioneerQwen35DeviceGraphRun(*descriptor,mailbox)==PioneerQwen35::DataflowGraphLocalComplete;
}
