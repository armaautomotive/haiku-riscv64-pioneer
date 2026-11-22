/* SPDX-License-Identifier: MIT */
#include "PioneerQwen35DataflowBlackholeWorker.h"

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
// The graph code remains non-loadable. Its local record check is linked from
// the dispatch witness; the kernel must establish sealed model ownership.
extern "C" bool qwen35_graph_execute(unsigned core,unsigned sequence,
	PioneerQwen35::DataflowWorkerMailbox* mailbox,unsigned* detail)
{
	const auto result=PioneerQwen35::DataflowWorkerExecuteAuthorized(core,sequence,mailbox);
	if (detail) *detail=result.executionDetail;
	return result.okay;
}
extern "C" bool qwen35_graph_reset_queues() {
	return PioneerQwen35::DataflowWorkerResetQueues();
}
#endif

extern "C" [[noreturn]] void qwen35_dataflow_service_main()
{
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
	// The role graph accesses shared L1 before its first projection too.
	// Do not enter the service if the non-coherent cache cannot be bypassed.
	uint32_t cfg0;
	asm volatile("fence iorw,iorw\n\tcsrsi 0x7c0,8\n\tcsrr %0,0x7c0"
		: "=r"(cfg0) :: "memory");
	if (!(cfg0&8U)) for (;;) asm volatile("" ::: "memory");
#endif
	PioneerQwen35::PioneerQwen35DataflowBlackholeWorkerService();
}
