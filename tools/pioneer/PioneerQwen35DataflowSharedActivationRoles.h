/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_SHARED_ACTIVATION_ROLES_H
#define PIONEER_QWEN35_DATAFLOW_SHARED_ACTIVATION_ROLES_H
#include "PioneerQwen35DataflowKernels.h"
#include "PioneerQwen35DataflowActivationReuse.h"
#include "PioneerQwen35DataflowActivationSums.h"
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
#include "PioneerQwen35DataflowActivationShareProtocol.h"
#endif
namespace PioneerQwen35 {
// Offline multi-role variant: the reader packs the immutable activation once,
// publishes one input-ready ticket, then streams only weight pages. Compute
// holds that ticket while directly reusing packed chunks in the shared L1
// cache. The coordinator must not reuse this memory before every role and
// compute completes. This is not a cache-coherence or launch proof.
template<class Ops>
bool PioneerQwen35SharedActivationReader(const DataflowCoreInvocation& program,
    DataflowQueueView& weights,DataflowQueueView& ready,uint8_t* activationCache,Ops& ops,
    DataflowActivationReuse* reuse=nullptr)
{
    typename Ops::WeightPipeline pipeline(ops);
    bool pipelineStarted=false;
    auto fail=[&]() {
        // Try to retire each known request exactly once. An uncertain issue or
        // failed completion poisons the pipeline and leaves the role faulted;
        // neither its pages nor its NoC are safe to reuse on that path.
        if (pipelineStarted) {
            while (pipeline.Retired()<pipeline.Issued())
                if (!pipeline.Complete(pipeline.Retired())) break;
            if (pipeline.Retired()==pipeline.Issued()) (void)pipeline.Finish();
        }
        DataflowQueueFail(weights,DataflowFailureReader);
        DataflowQueueFail(ready,DataflowFailureReader);
        return false;
    };
    if (!DataflowCoreHeaderValid(program) || !DataflowQueueValid(weights)
        || !DataflowQueueValid(ready) || !activationCache || !ops.Bind(program)) return fail();
    if (!program.assignment.active) { if (reuse) DataflowStoreRelease(reuse->magic,0);return true; }
    const bool reusable=reuse && DataflowActivationReuseHit(*reuse,program);
    if (reuse) DataflowStoreRelease(reuse->magic,0);
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
    DataflowActivationShareTreePosition share{};
    if (!reuse || !DataflowActivationShareTreeLocate(program.sharing,program.core,share)) return fail();
    const bool readLocal=share.parent==DataflowCoreCount;
#else
    const bool readLocal=true;
#endif
    if (!reusable && readLocal && !ops.ReadActivation(program.input,program.inputRows,activationCache,
            program.inputRows*sizeof(float))) return fail();
    const unsigned tileBytes=program.circularBuffer[0].pageBytes;
    const uint64_t total64=uint64_t(program.assignment.groups)*program.blocks;
    if (!tileBytes || !total64 || total64>UINT32_MAX-2U
        || DataflowLoadAcquire(weights.control->produced)
        || DataflowLoadAcquire(weights.control->consumed) || !pipeline.Start()) return fail();
    pipelineStarted=true;
    const uint32_t total=uint32_t(total64);
    unsigned segmentIndex=0,group=0,block=0;
    uint32_t issued=0,completed=0;
    auto issueNext=[&]() {
        if (issued>=total || segmentIndex>=program.assignment.segmentCount) return false;
        const auto& segment=program.assignment.weight[segmentIndex];
        const uint64_t groupBytes=uint64_t(16)*program.weights.rowBytes;
        const uint64_t groupOffset=uint64_t(group)*groupBytes;
        const uint64_t blockOffset=uint64_t(block)*tileBytes;
        const uint64_t address=uint64_t(segment.address)+groupOffset+blockOffset;
        if (group>=segment.groups || address>UINT32_MAX || groupOffset>segment.bytes
            || groupBytes>segment.bytes-groupOffset || blockOffset>groupBytes
            || tileBytes>groupBytes-blockOffset) return false;
        uint8_t* const page=weights.local+(issued%weights.layout->pages)*weights.layout->pageStride;
        if (!pipeline.Issue(issued,segment.bank,uint32_t(address),page,tileBytes)) return false;
        issued++;
        if (++block==program.blocks) {
            block=0;
            if (++group==segment.groups) { group=0;segmentIndex++; }
        }
        return true;
    };
    // Start one transfer while packing the activation; issue the second into
    // the other queue page before waiting for the first completion.
    if (!issueNext()) return fail();
    if (!reusable && readLocal && (!DataflowPackActivationCache(program,ready,activationCache,ops)
        || !DataflowPrepareActivationSums(activationCache,program.type,program.inputs))) return fail();
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
    if ((!reusable && !readLocal && !ops.ReceiveSharedActivation(program,*reuse,activationCache))
        || !ops.PublishSharedActivation(program,*reuse)) return fail();
#endif
    uint32_t readyTicket=0;
    if (!DataflowQueueReserveBack(ready,readyTicket) || readyTicket!=0
        || !DataflowQueuePublishBack(ready,readyTicket)) return fail();
    while (completed<total) {
        uint32_t consumed=DataflowLoadAcquire(weights.control->consumed);
        if (DataflowQueueFailure(*weights.control) || DataflowQueueFailure(*ready.control)
            || DataflowLoadRelaxed(weights.control->produced)!=completed
            || completed-consumed>weights.layout->pages
            || issued-consumed>weights.layout->pages) return fail();
        while (issued<total && issued-consumed<weights.layout->pages) {
            if (DataflowQueueFailure(*weights.control) || DataflowQueueFailure(*ready.control)
                || !issueNext()) return fail();
            consumed=DataflowLoadAcquire(weights.control->consumed);
            if (completed-consumed>weights.layout->pages
                || issued-consumed>weights.layout->pages) return fail();
        }
        if (completed==issued) {
            // All issued pages are already published and the consumer has not
            // freed a slot. Preserve the existing bounded queue wait policy.
            uint32_t ticket=0;
            if (!DataflowQueueReserveBack(weights,ticket) || ticket!=completed) return fail();
            continue;
        }
        if (!pipeline.Complete(completed) || !DataflowQueuePublishBack(weights,completed))
            return fail();
        completed++;
    }
    if (issued!=total || segmentIndex!=program.assignment.segmentCount
        || group || block || !pipeline.Finish()) return fail();
    pipelineStarted=false;

    if (reuse) DataflowActivationReuseCommit(*reuse,program);
    return true;
}

template<class Ops>
bool PioneerQwen35SharedActivationCompute(const DataflowCoreInvocation& program,
    DataflowQueueView& weights,DataflowQueueView& ready,DataflowQueueView& outputs,
    const uint8_t* activationCache,Ops& ops)
{
    static_assert(Ops::TensixMatrixEngine,"Qwen35 projections require the Tensix matrix engine");
    auto fail=[&]() {
        DataflowQueueFail(weights,DataflowFailureCompute);
        DataflowQueueFail(ready,DataflowFailureCompute);
        DataflowQueueFail(outputs,DataflowFailureCompute);
        return false;
    };
    if (!DataflowCoreHeaderValid(program) || !DataflowQueueValid(weights)
        || !DataflowQueueValid(ready) || !DataflowQueueValid(outputs)
        || !activationCache || !ops.Bind(program)) return fail();
    if (!program.assignment.active) return true;
    uint32_t readyTicket=0; uint8_t* readyPage=nullptr;
    if (!DataflowQueueWaitFront(ready,readyTicket,readyPage) || readyTicket!=0 || !readyPage)
        return fail();
    for (unsigned segmentIndex=0;segmentIndex<program.assignment.segmentCount;segmentIndex++) {
        const auto& segment=program.assignment.weight[segmentIndex];
        for (unsigned group=0;group<segment.groups;group++) {
            if (!ops.TensixBeginOutput(program.type)) return fail();
            for (unsigned block=0;block<program.blocks;block++) {
                uint32_t ticket=0; uint8_t* weightPage=nullptr;
                const uint8_t* inputPage=DataflowPackedActivationPage(
                    activationCache,program.type,program.inputs,block);
                if (!inputPage || !DataflowQueueWaitFront(weights,ticket,weightPage)
                    || !weightPage || !ops.TensixMatmul(program.type,block,weightPage,inputPage)
                    || !DataflowQueuePopFront(weights,ticket)) return fail();
            }
            uint32_t outputTicket=0;
            if (!DataflowQueueReserveBack(outputs,outputTicket)) return fail();
            uint8_t* outputPage=DataflowQueueWritePage(outputs,outputTicket);
            if (!outputPage || !ops.TensixFinishOutput(outputPage,program.circularBuffer[2].pageBytes)
                || !DataflowQueuePublishBack(outputs,outputTicket)) return fail();
        }
    }
    return DataflowQueuePopFront(ready,readyTicket) || fail();
}

// Coalesce only pages that are already ready, adjacent in both the circular
// buffer and the same destination segment. Never wait for a second page:
// the writer remains concurrent with compute even for a one-group shard.
template<class Ops>
bool PioneerQwen35SharedActivationWriter(const DataflowCoreInvocation& program,
    DataflowQueueView& outputs,Ops& ops)
{
    auto fail=[&]() { DataflowQueueFail(outputs,DataflowFailureWriter);return false; };
    if (!DataflowCoreHeaderValid(program) || !DataflowQueueValid(outputs)
        || !ops.Bind(program)) return fail();
    for (unsigned s=0;s<program.assignment.segmentCount;s++) {
        const auto& segment=program.assignment.output[s];
        const unsigned groups=segment.rows/16;
        for (unsigned group=0;group<groups;) {
            uint32_t ticket=0;uint8_t* page=nullptr;
            const uint64_t address=uint64_t(segment.address)
                +uint64_t(group)*DataflowOutputPageBytes;
            if (!DataflowQueueWaitFront(outputs,ticket,page) || !page
                || address>UINT32_MAX) return fail();
            const uint32_t produced=DataflowLoadAcquire(outputs.control->produced);
            const uint32_t available=produced-ticket;
            if (!available || available>outputs.layout->pages) return fail();
            const bool pair=group+1<groups && available>=2
                && ticket%outputs.layout->pages==0
                && outputs.layout->pageStride==DataflowOutputPageBytes;
            if (pair) {
                if (address>UINT32_MAX-DataflowOutputPageBytes
                    || !ops.WriteOutputPair(segment.bank,uint32_t(address),page)
                    || !DataflowQueuePopFront(outputs,ticket)
                    || !DataflowQueuePopFront(outputs,ticket+1)) return fail();
                group+=2;
            } else {
                if (!ops.WriteOutput(segment.bank,uint32_t(address),page,
                        DataflowOutputPageBytes)
                    || !DataflowQueuePopFront(outputs,ticket)) return fail();
                group++;
            }
        }
    }
    return true;
}
}
#endif
