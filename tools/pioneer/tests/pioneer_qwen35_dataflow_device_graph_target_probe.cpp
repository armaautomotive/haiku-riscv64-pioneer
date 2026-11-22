/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowDeviceGraph.h"
#include "../PioneerQwen35DataflowDynamicToken.h"
#include "../PioneerQwen35DataflowGraphRunProtocol.h"

// Only linked into the non-loadable device-dispatch witness. The linked
// authorization hook currently rejects all commands, and the resident
// service does not call this entry. Do not expose it as a card operation.
extern "C" bool qwen35_graph_authorize(unsigned core,unsigned index);
extern "C" bool qwen35_graph_execute(unsigned core,unsigned sequence,
	PioneerQwen35::DataflowWorkerMailbox* mailbox);
extern "C" bool qwen35_graph_reset_queues();

struct GraphWitnessExecution {
	const PioneerQwen35::DataflowStreamBinding& binding;
	const PioneerQwen35::Range& lease;
	const unsigned (&routes)[2][PioneerQwen35::Banks];
	const PioneerQwen35::DataflowDynamicTokenContext& run;
	unsigned core;
	PioneerQwen35::DataflowWorkerMailbox* mailbox;
	bool Fetch(unsigned index) {
		return PioneerQwen35::DataflowBlackholeStreamFetchDynamic(binding,lease,routes,core,index,run);
	}
	bool Authorize(unsigned index) { return qwen35_graph_authorize(core,index); }
	bool Execute(unsigned sequence,unsigned& status,unsigned& detail) {
		status=detail=0;return qwen35_graph_execute(core,sequence,mailbox);
	}
	bool ResetQueues() { return qwen35_graph_reset_queues(); }
};
extern "C" unsigned PioneerQwen35DeviceGraphCompileProbe(
	const PioneerQwen35::DataflowStreamBinding& binding,const PioneerQwen35::Range& lease,
	const unsigned (&routes)[2][PioneerQwen35::Banks],unsigned core,unsigned pollBudget,
	unsigned runEpoch,unsigned sequenceStart,
	const PioneerQwen35::DataflowDynamicTokenContext& run,
	PioneerQwen35::DataflowWorkerMailbox* mailbox)
{
	GraphWitnessExecution execution{binding,lease,routes,run,core,mailbox};
	PioneerQwen35::DataflowBlackholeBarrierIO transport(core);
	PioneerQwen35::DataflowDeviceGraph<GraphWitnessExecution,
		PioneerQwen35::DataflowBlackholeBarrierIO> graph(execution,transport);
	if (!graph.Start(binding,core,pollBudget,runEpoch,sequenceStart)) return PioneerQwen35::DataflowGraphInvalid;
	for (;;) {
		const auto state=graph.Tick();
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
	return PioneerQwen35DeviceGraphCompileProbe(descriptor->binding,descriptor->lease,
		descriptor->routes,descriptor->core,descriptor->pollLimit,descriptor->ticket.epoch,
		descriptor->ticket.firstSequence,descriptor->token,mailbox)==PioneerQwen35::DataflowGraphLocalComplete;
}
