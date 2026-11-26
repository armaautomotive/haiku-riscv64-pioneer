/* SPDX-License-Identifier: MIT */
#include "PioneerQwen35DataflowBlackholeWorker.h"
#ifdef PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY
#include "PioneerQwen35DataflowBlackholeFlashHybridIO.h"
#endif

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
#ifdef PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY
// This graph-local admission checks only the already-owned worker and role
// state. The non-loadable caller must still own the sealed model, both NoCs,
// activation cache, all workers and graph barriers for the entire operation.
struct Q35FlashGraphContext {
    const PioneerQwen35::DataflowGraphRunDescriptor& run;
    const PioneerQwen35::DataflowFlashHybridInvocation& invocation;
    PioneerQwen35::DataflowWorkerMailbox& box;
    const PioneerQwen35::DataflowActivationReuse& reuse;
    const PioneerQwen35::DataflowRoleMailbox& roles;
    unsigned index,sequence;
    bool FlashContextMatches(unsigned core,unsigned epoch,unsigned step,unsigned layer,unsigned position) const {
        using namespace PioneerQwen35;
        return core==run.core && core==invocation.core && box.logicalCore==core
            && epoch==run.ticket.epoch && step==index+1 && layer==invocation.program.layer
            && position==run.token.position && position==invocation.program.position
            && sequence==run.ticket.firstSequence+index && index<run.ticket.count
            && DataflowLoadAcquire(box.commandSequence)==run.triggerSequence
            && DataflowLoadAcquire(box.state)==DataflowWorkerRunning
            && !DataflowLoadAcquire(box.shutdownSequence) && !DataflowLoadAcquire(box.failure)
            && DataflowLoadAcquire(reuse.epoch)==epoch && DataflowLoadAcquire(reuse.step)==step
            && DataflowWorkerQueuesClean() && DataflowRoleMailboxIdle(roles)
            && DataflowLoadAcquire(reuse.epoch)==epoch && DataflowLoadAcquire(reuse.step)==step;
    }
};
extern "C" __attribute__((noinline)) bool qwen35_graph_flash_execute(
    const PioneerQwen35::DataflowGraphRunDescriptor* run,unsigned index,unsigned sequence,
    PioneerQwen35::DataflowWorkerMailbox* box,unsigned* detail) {
    using namespace PioneerQwen35;
    if (detail) *detail=DataflowFailureReader;
    if (!run || !box || !DataflowGraphRunValid(*run,box->logicalCore) || index>=run->ticket.count
        || sequence!=run->ticket.firstSequence+index) return false;
    const auto* invocation=reinterpret_cast<const DataflowFlashHybridInvocation*>(
        static_cast<uintptr_t>(DataflowProgramAddress));
    if (!invocation->Valid() || invocation->core!=run->core
        || invocation->program.position!=run->token.position) return false;
    auto* reuse=reinterpret_cast<DataflowActivationReuse*>(static_cast<uintptr_t>(DataflowActivationReuseAddress));
    const auto* roles=reinterpret_cast<const DataflowRoleMailbox*>(static_cast<uintptr_t>(DataflowRoleMailboxAddress));
    Q35FlashGraphContext context{*run,*invocation,*box,*reuse,*roles,index,sequence};
    const unsigned step=index+1,epoch=run->ticket.epoch,layer=invocation->program.layer,position=run->token.position;
    if (!context.FlashContextMatches(run->core,epoch,step,layer,position)) return false;
    auto* receipt=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(DataflowFlashPartialLocalReceipt));
    const bool reduce=invocation->phase==DataflowFlashHybridReducer;
    unsigned producerStep=0;
    if (!DataflowFlashPartialProducerStep(step,reduce,producerStep)) return false;
    if (reduce) {
        if (!DataflowFlashProducerReceiptMatches(receipt,epoch,producerStep,layer,position,run->core)) return false;
    } else {
        DataflowFlashProducerReceiptClear(receipt);
        // The hybrid scratch deliberately replaces projection activations.
        DataflowStoreRelease(reuse->magic,0);
    }
    DataflowBlackholeFlashHybridIO<Q35FlashGraphContext> io(context,run->core,epoch,step,reduce);
    const auto scratch=DataflowBlackholeFlashHybridScratch();
    const bool okay=reduce?DataflowFlashHybridReduce(invocation->program,run->core,scratch,io)
        :DataflowFlashHybridProduce(invocation->program,run->core,scratch,io);
    if (!okay || !context.FlashContextMatches(run->core,epoch,step,layer,position)) return false;
    if (!reduce && !DataflowFlashProducerReceiptPublish(receipt,epoch,step,layer,position,run->core)) return false;
    if (detail) *detail=0;
    return true;
}
#endif
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
