/* SPDX-License-Identifier: MIT */
#include "PioneerQwen35DataflowBlackholeWorker.h"

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
// This is a non-loadable link witness. No model-bound authority exists yet,
// so its graph authorization hook must reject every command.
extern "C" bool qwen35_graph_authorize(unsigned, unsigned) { return false; }
extern "C" bool qwen35_graph_execute(unsigned core,unsigned sequence,
	PioneerQwen35::DataflowWorkerMailbox* mailbox)
{
	return PioneerQwen35::DataflowWorkerExecuteAuthorized(core,sequence,mailbox).okay;
}
extern "C" bool qwen35_graph_reset_queues() {
	return PioneerQwen35::DataflowWorkerResetQueues();
}
#endif

extern "C" [[noreturn]] void qwen35_dataflow_service_main()
{
	PioneerQwen35::PioneerQwen35DataflowBlackholeWorkerService();
}
