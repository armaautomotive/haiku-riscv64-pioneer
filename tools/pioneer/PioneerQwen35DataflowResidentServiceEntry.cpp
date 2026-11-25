/* SPDX-License-Identifier: MIT */
// Resident shared-role graph entrypoint, compiled as an offline witness only.
// The caller must install a validated per-core template and retain exclusive
// ownership; no loadable driver or host submission path invokes this yet.
#if !defined(PIONEER_QWEN35_DEVICE_DISPATCH) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) \
    || !defined(PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS)
#error Resident graph entry requires the non-loadable shared-role witness
#endif
#include "PioneerQwen35DataflowResidentServiceIO.h"
using namespace PioneerQwen35;

[[noreturn]] static void ResidentParkFault(DataflowWorkerMailbox& box) {
    const unsigned state=DataflowLoadAcquire(box.state);
    if (state!=DataflowWorkerFailed && state!=DataflowWorkerFatal)
        DataflowWorkerPublish(box,DataflowLoadAcquire(box.completedSequence),DataflowWorkerFatal,DataflowFailureQueue,6);
    DataflowFence();for (;;) asm volatile("" ::: "memory");
}
extern "C" [[noreturn]] void qwen35_resident_graph_service_main() {
    // The shared graph accesses L1 before its first projection. Match the
    // existing service's non-coherent L0 cache bypass before any L1 access.
    uint32_t cfg0;
    asm volatile("fence iorw,iorw\n\tcsrsi 0x7c0,8\n\tcsrr %0,0x7c0"
        : "=r"(cfg0) :: "memory");
    if (!(cfg0&8U)) for (;;) asm volatile("" ::: "memory");
    auto* box=reinterpret_cast<DataflowWorkerMailbox*>(static_cast<uintptr_t>(DataflowWorkerMailboxAddress));
    auto* roles=reinterpret_cast<const DataflowRoleMailbox*>(static_cast<uintptr_t>(DataflowRoleMailboxAddress));
    auto* queues=reinterpret_cast<DataflowQueueControl*>(static_cast<uintptr_t>(DataflowQueueStateAddress));
    if (!DataflowWorkerMailboxStartupValid(*box)) ResidentParkFault(*box);
    const unsigned core=box->logicalCore;
    DataflowResidentSenderReceiptClear(reinterpret_cast<volatile uint32_t*>(
        static_cast<uintptr_t>(DataflowResidentSenderReceiptAddress)));
    auto* cache=reinterpret_cast<DataflowResidentStreamCache*>(static_cast<uintptr_t>(DataflowResidentStreamCacheAddress));
    DataflowResidentStreamCacheReset(*cache);
    const uint32_t raw=*reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(DataflowNoc0Base+DataflowNocLogicalIdOffset));
    uint32_t route=0;
    if (!DataflowNocDecodeLocalRoute(raw,route) || !DataflowNocRouteMatchesWorker(route,core)) ResidentParkFault(*box);
    // The per-core static binding is installed and authenticated before boot.
    // Dynamic token fields are rebuilt locally from the coordinator request.
    DataflowGraphRunDescriptor installed{};
    const auto* source=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(DataflowGraphRunAddress));
    auto* bytes=reinterpret_cast<uint8_t*>(&installed);
    for (unsigned i=0;i<sizeof(installed)/4;i++) {
        const uint32_t word=DataflowLoadAcquire(source[i]);
        for (unsigned b=0;b<4;b++) bytes[4*i+b]=uint8_t(word>>(8*b));
    }
    if (!DataflowGraphRunValid(installed,core)) ResidentParkFault(*box);
    auto* receipt=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(DataflowResidentCandidateAddress));
    auto* ack=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(DataflowResidentCandidateAckAddress));
    if (!core) {
        DataflowStoreRelease(receipt[7],0);
        for (unsigned i=0;i<4;i++) DataflowStoreRelease(ack[i],0);
        DataflowFence();
    }
    unsigned completed=0,epoch=0,lastStep=0;
    bool waitingForAudit=false,pendingOutput=false;
    for (;;) {
        // The root forwards no later command until the locked host owner
        // acknowledges this exact candidate after its independent fault audit.
        if (!core && waitingForAudit) {
            if (!DataflowResidentCandidateAckMatches(ack,completed,epoch)) continue;
            if (pendingOutput) {
                DataflowArgmaxResult output{};
                if (!DataflowArgmaxReadResult(*box,lastStep,output)) ResidentParkFault(*box);
                DataflowWorkerClearReceipt(*box);DataflowFence();pendingOutput=false;
            }
            waitingForAudit=false;
        }
        const unsigned shutdown=DataflowLoadAcquire(box->shutdownSequence);
        if (shutdown) {
            if (shutdown!=DataflowWorkerShutdownRequest || !DataflowWorkerStopRequestValid(*box)
                || !DataflowWorkerResetQueues()) ResidentParkFault(*box);
            DataflowWorkerPublish(*box,completed,DataflowWorkerStopped,0,0);DataflowFence();
            asm volatile(".globl qwen35_resident_graph_stopped_park\n"
                "qwen35_resident_graph_stopped_park:\n"
                "j qwen35_resident_graph_stopped_park\n" ::: "memory");
            __builtin_unreachable();
        }
        DataflowResidentServiceIO io(core,*box,*roles,queues);
        DataflowResidentServiceRun<DataflowResidentServiceIO,DataflowBlackholeBarrierIO>
            run(io,core,installed,completed,epoch,lastStep);
        auto status=run.Tick();
        while (status==DataflowResidentServicePending) status=run.Tick();
        if (status==DataflowResidentServiceIdle) continue;
        if (status!=DataflowResidentServiceCandidate) ResidentParkFault(*box);
        completed=run.CompletedSequence();epoch=run.CompletedEpoch();lastStep=run.CompletedLastStep();
        if (!core) { waitingForAudit=true;pendingOutput=run.CompletedHasOutput(); }
    }
}
