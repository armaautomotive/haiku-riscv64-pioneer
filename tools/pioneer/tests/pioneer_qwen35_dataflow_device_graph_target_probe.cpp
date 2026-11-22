/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowDeviceGraph.h"

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
	unsigned core;
	PioneerQwen35::DataflowWorkerMailbox* mailbox;
	bool Fetch(unsigned index) {
		return PioneerQwen35::DataflowBlackholeStreamFetch(binding,lease,routes,core,index);
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
	PioneerQwen35::DataflowWorkerMailbox* mailbox)
{
	GraphWitnessExecution execution{binding,lease,routes,core,mailbox};
	PioneerQwen35::DataflowBlackholeBarrierIO transport(core);
	PioneerQwen35::DataflowDeviceGraph<GraphWitnessExecution,
		PioneerQwen35::DataflowBlackholeBarrierIO> graph(execution,transport);
	if (!graph.Start(binding,core,pollBudget)) return PioneerQwen35::DataflowGraphInvalid;
	for (;;) {
		const auto state=graph.Tick();
		if (state==PioneerQwen35::DataflowGraphLocalComplete || state==PioneerQwen35::DataflowGraphRetain)
			return state;
	}
}
