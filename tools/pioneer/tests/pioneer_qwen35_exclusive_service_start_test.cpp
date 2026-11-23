/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS 1
#define main qwen35_original_service_test_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#include <memory>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowPciMemory.h"

struct DedicatedIO : MockDataflowIO {
    unsigned releaseAttempts=0;
    bool allowStop=false;
    unsigned failHoldCore=UINT32_MAX;
    bool CaptureCoreState(unsigned)=delete;
    bool HoldCore(unsigned core) {
        if (core>=DataflowCoreCount) return false;
        held[core]=true;started[core]=false;
        return core!=failHoldCore; // a failed write may still have taken effect
    }
    bool SharedCaptureAndStageHeld() {
        ++roleStages;
        for (unsigned core=0;core<DataflowCoreCount;core++) if (!held[core]) return false;
        roleStaged=true;return !failRoleStage;
    }
    bool ReadCoreMemory(unsigned core,unsigned address,void* output,unsigned bytes) {
        assert(address!=DataflowRoleSnapshotHandoffAddress);
        return MockDataflowIO::ReadCoreMemory(core,address,output,bytes);
    }
    bool WriteCoreMemory(unsigned core,unsigned address,const void* input,unsigned bytes) {
        assert(address!=DataflowRoleSnapshotHandoffAddress);
        return MockDataflowIO::WriteCoreMemory(core,address,input,bytes);
    }
    bool SharedStartCore(unsigned core) {
        assert(core==releaseAttempts++ && roleArmed);
        if (!StartCore(core)) return false;
        roleStarted[core]=true;++roleStarts;
        return DataflowRoleMailboxPublishStarted(roleMailbox[core],DataflowRoleReader)
            && DataflowRoleMailboxPublishStarted(roleMailbox[core],DataflowRoleWriter);
    }
    // Compilation itself proves that the dedicated service doesn't attempt
    // the snapshot shim's old per-core stack handoff.
    bool SharedPrepareRoleStacks(unsigned)=delete;
    bool SharedStopAndHoldAll(unsigned& stage,unsigned& core) {
        stage=17;core=releaseAttempts?releaseAttempts-1:DataflowCoreCount;
        ++roleStops;return allowStop;
    }
    bool SharedTakeoverResetRequired() { return false; }
};
int main() {
    assert(!DataflowPciRunningWriteRange(DataflowRoleSnapshotHandoffAddress
        +offsetof(DataflowRoleSnapshotHandoff,release),sizeof(uint32_t)));
    uint32_t text[32]{},dispatch[16]{};
    const DataflowServiceImageView image={DataflowServiceBootWord,text,32,dispatch,16};
    {
        auto io=std::make_unique<DedicatedIO>();auto owner=std::make_unique<DataflowServiceOwner>();
        assert(owner->Install(image,*io)==DataflowServiceComplete && owner->Ready());
        assert(io->releaseAttempts==DataflowCoreCount && io->roleStarts==DataflowCoreCount && !io->captureCount);
        for (unsigned i=0;i<DataflowCoreCount;i++) assert(!io->roleHandoff[i].release);
        assert(owner->Close(*io)==DataflowServiceStopAuditFailed && io->restoreCount==0);
        assert(owner->SharedDedicatedCleanStopPending());
        io->allowStop=true;
        assert(owner->Close(*io)==DataflowServiceResetRequired && owner->SharedDedicatedCleanStopPending());
    }
    {
        auto io=std::make_unique<DedicatedIO>();auto owner=std::make_unique<DataflowServiceOwner>();
        io->failHoldCore=3;
        assert(owner->Install(image,*io)==DataflowServiceHoldFailed && owner->Faulted());
        assert(io->held[3] && !io->captureCount && !io->roleStages && !io->releaseAttempts);
        assert(!owner->SharedDedicatedCleanStopPending());
        assert(owner->Close(*io)==DataflowServiceResetRequired && owner->StoppedResetReady());
        assert(io->stopCount==1 && !io->restoreCount && !io->resumeCount);
    }
    {
        auto io=std::make_unique<DedicatedIO>();auto owner=std::make_unique<DataflowServiceOwner>();
        io->failRoleStage=true;io->allowStop=true;
        assert(owner->Install(image,*io)==DataflowServiceSnapshotFailed && owner->Faulted());
        assert(!io->captureCount && !io->releaseAttempts);
        assert(owner->Close(*io)==DataflowServiceResetRequired && owner->StoppedResetReady());
        assert(!io->restoreCount && !io->resumeCount);
    }
    {
        auto io=std::make_unique<DedicatedIO>();auto owner=std::make_unique<DataflowServiceOwner>();
        assert(owner->Install(image,*io)==DataflowServiceComplete);io->allowStop=true;
        assert(owner->Close(*io)==DataflowServiceResetRequired && owner->StoppedResetReady());
        assert(owner->Close(*io)==DataflowServiceResetRequired && io->roleStops==1 && !io->restoreCount);
    }
    {
        auto io=std::make_unique<DedicatedIO>();auto owner=std::make_unique<DataflowServiceOwner>();
        io->failStartCore=3;
        assert(owner->Install(image,*io)==DataflowServiceStartFailed);
        assert(io->releaseAttempts==4 && io->roleStarts==3);
        assert(owner->Close(*io)==DataflowServiceStopAuditFailed && io->restoreCount==0);
        io->allowStop=true;
        assert(owner->Close(*io)==DataflowServiceResetRequired && owner->StoppedResetReady());
    }
    std::puts("PASS dedicated service starts 120 workers without snapshot handoffs and retains failed starts; SIMULATED ONLY");
}
