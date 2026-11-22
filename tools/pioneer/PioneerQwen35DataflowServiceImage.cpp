/* SPDX-License-Identifier: MIT */
#include "PioneerQwen35DataflowBlackholeWorker.h"

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
// The graph code remains non-loadable. Its local record check is linked from
// the dispatch witness; the kernel must establish sealed model ownership.
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
