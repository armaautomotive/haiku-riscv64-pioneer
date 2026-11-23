/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_SHARED_ACTIVATION_ROLES_H
#define PIONEER_QWEN35_DATAFLOW_SHARED_ACTIVATION_ROLES_H
#include "PioneerQwen35DataflowKernels.h"
namespace PioneerQwen35 {
// Offline multi-role variant: the reader packs the immutable activation once,
// publishes one input-ready ticket, then streams only weight pages. Compute
// holds that ticket while directly reusing packed chunks in the shared L1
// cache. The coordinator must not reuse this memory before every role and
// compute completes. This is not a cache-coherence or launch proof.
template<class Ops>
bool PioneerQwen35SharedActivationReader(const DataflowCoreInvocation& program,
    DataflowQueueView& weights,DataflowQueueView& ready,uint8_t* activationCache,Ops& ops)
{
    // BeginWeightRead fills the command/counter fields before setting active.
    // Initialize only that field so the freestanding role needs no memset.
    typename Ops::WeightReadTicket firstRead;
    firstRead.active=0;
    bool firstReadPending=false;
    auto fail=[&]() {
        // If packing or publication fails after issuing the prefetch, try to
        // drain it once. A failed drain remains a retained NoC fault; never
        // publish or reuse the page on that path.
        if (firstReadPending) {
            firstReadPending=false;
            (void)ops.CompleteWeightRead(firstRead);
        }
        DataflowQueueFail(weights,DataflowFailureReader);
        DataflowQueueFail(ready,DataflowFailureReader);
        return false;
    };
    if (!DataflowCoreHeaderValid(program) || !DataflowQueueValid(weights)
        || !DataflowQueueValid(ready) || !activationCache || !ops.Bind(program)) return fail();
    if (!program.assignment.active) return true;
    if (!ops.ReadActivation(program.input,program.inputRows,activationCache,
            program.inputRows*sizeof(float))) return fail();
    const unsigned tileBytes=program.circularBuffer[0].pageBytes;
    const auto& firstWeight=program.assignment.weight[0];
    uint32_t firstTicket=0;
    if (!DataflowQueueReserveBack(weights,firstTicket) || firstTicket!=0) return fail();
    uint8_t* const firstPage=DataflowQueueWritePage(weights,firstTicket);
    if (!firstPage || tileBytes>firstWeight.bytes) return fail();
    // Record the possible transfer before invoking the transport. If an
    // implementation reports failure after issuing it, fail() still attempts
    // the one allowed drain before returning the retained fault.
    firstReadPending=true;
    if (!ops.BeginWeightRead(firstWeight.bank,firstWeight.address,firstPage,tileBytes,firstRead)) return fail();
    if (!DataflowPackActivationCache(program,ready,activationCache,ops)) return fail();
    uint32_t readyTicket=0;
    if (!DataflowQueueReserveBack(ready,readyTicket) || readyTicket!=0
        || !DataflowQueuePublishBack(ready,readyTicket)) return fail();
    firstReadPending=false; // Complete is attempted once, even if its result is uncertain.
    if (!ops.CompleteWeightRead(firstRead) || !DataflowQueuePublishBack(weights,firstTicket)) return fail();
    for (unsigned segmentIndex=0;segmentIndex<program.assignment.segmentCount;segmentIndex++) {
        const auto& segment=program.assignment.weight[segmentIndex];
        for (unsigned group=0;group<segment.groups;group++) {
            const uint64_t groupBytes=uint64_t(16)*program.weights.rowBytes;
            const uint64_t groupOffset=uint64_t(group)*groupBytes;
            for (unsigned block=0;block<program.blocks;block++) {
                if (!segmentIndex && !group && !block) continue; // prefetched above
                uint32_t ticket=0;
                if (!DataflowQueueReserveBack(weights,ticket)) return fail();
                uint8_t* page=DataflowQueueWritePage(weights,ticket);
                const uint64_t blockOffset=uint64_t(block)*tileBytes;
                const uint64_t address=uint64_t(segment.address)+groupOffset+blockOffset;
                if (!page || address>UINT32_MAX || groupOffset>segment.bytes
                    || groupBytes>segment.bytes-groupOffset || blockOffset>groupBytes
                    || tileBytes>groupBytes-blockOffset
                    || !ops.ReadWeight(segment.bank,uint32_t(address),page,tileBytes)
                    || !DataflowQueuePublishBack(weights,ticket)) return fail();
            }
        }
    }
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
}
#endif
