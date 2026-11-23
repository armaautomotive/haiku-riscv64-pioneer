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
        ++roleStops;return false; // physical dedicated stop is not wired yet
    }
};
int main() {
    assert(!DataflowPciRunningWriteRange(DataflowRoleSnapshotHandoffAddress
        +offsetof(DataflowRoleSnapshotHandoff,release),sizeof(uint32_t)));
    uint32_t text[32]{},dispatch[16]{};
    const DataflowServiceImageView image={DataflowServiceBootWord,text,32,dispatch,16};
    {
        auto io=std::make_unique<DedicatedIO>();auto owner=std::make_unique<DataflowServiceOwner>();
        assert(owner->Install(image,*io)==DataflowServiceComplete && owner->Ready());
        assert(io->releaseAttempts==DataflowCoreCount && io->roleStarts==DataflowCoreCount);
        for (unsigned i=0;i<DataflowCoreCount;i++) assert(!io->roleHandoff[i].release);
        assert(owner->Close(*io)==DataflowServiceStopAuditFailed && io->restoreCount==0);
    }
    {
        auto io=std::make_unique<DedicatedIO>();auto owner=std::make_unique<DataflowServiceOwner>();
        io->failStartCore=3;
        assert(owner->Install(image,*io)==DataflowServiceStartFailed);
        assert(io->releaseAttempts==4 && io->roleStarts==3);
        assert(owner->Close(*io)==DataflowServiceStopAuditFailed && io->restoreCount==0);
    }
    std::puts("PASS dedicated service starts 120 workers without snapshot handoffs and retains failed starts; SIMULATED ONLY");
}
