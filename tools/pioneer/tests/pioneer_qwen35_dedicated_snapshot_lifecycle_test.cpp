/* SPDX-License-Identifier: MIT */
// Compose the real held-fleet and snapshot-service owners with separate fake
// transports. This exercises their lifecycle; it does not model hardware IO.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define main snapshot_service_fixture_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
namespace DedicatedTest {
#define main snapshot_fleet_fixture_main
#include "pioneer_qwen35_dedicated_fleet_test.cpp"
#undef main
}
#include <memory>
int main()
{
    (void)&snapshot_service_fixture_main;(void)&DedicatedTest::snapshot_fleet_fixture_main;
    DedicatedTest::Dram dram;DedicatedTest::IO fleetIo;DedicatedTest::Banks banks;DedicatedTest::Lease lease;lease.worker[0].store.ready=true;
    assert(DedicatedTest::PioneerQwen35DedicatedFleetPrepare(fleetIo,dram,banks,lease));
    assert(lease.prepared==120 && !lease.booted && !lease.bootAttempted);
    auto io=std::make_unique<MockDataflowIO>();
    auto service=std::make_unique<DataflowServiceOwner>();
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        io->originalRunning[core]=false;io->started[core]=false;io->held[core]=true;
    }
    uint32_t code[96]{},dispatch[80]{};
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    assert(service->Install(image,*io)==DataflowServiceComplete);
    assert(io->captureCount==120 && io->roleStarts==120 && service->Ready());
    int stillMapped=1;
    assert(!DedicatedTest::PioneerQwen35DedicatedServiceReleased(service.get(),io.get(),&stillMapped));
    io->failRoleRestore=true;
    assert(service->Close(*io)==DataflowServiceRestoreFailed && service->Faulted());
    assert(!DedicatedTest::PioneerQwen35DedicatedServiceReleased(service.get(),static_cast<MockDataflowIO*>(nullptr),static_cast<int*>(nullptr)));
    assert(!lease.closed && !banks.quarantined && !fleetIo.restores);
    io->failRoleRestore=false;
    assert(service->Close(*io)==DataflowServiceClosed && service->Closed());
    assert(io->restoreCount==120 && io->resumeCount==1 && io->roleVerifies==1);
    for (unsigned core=0;core<DataflowCoreCount;core++)
        assert(io->held[core] && !io->started[core] && io->released[core]);
    assert(!DedicatedTest::PioneerQwen35DedicatedServiceReleased(service.get(),io.get(),static_cast<int*>(nullptr)));
    assert(!DedicatedTest::PioneerQwen35DedicatedServiceReleased(service.get(),static_cast<MockDataflowIO*>(nullptr),&stillMapped));
    assert(DedicatedTest::PioneerQwen35DedicatedServiceReleased(service.get(),static_cast<MockDataflowIO*>(nullptr),static_cast<int*>(nullptr)));
    fleetIo.audits=0;
    assert(DedicatedTest::PioneerQwen35DedicatedFleetClose(fleetIo,dram,banks,lease));
    assert(lease.closed && fleetIo.restores==119 && !banks.quarantined);
    std::puts("PASS dedicated fleet plus snapshot service clean close and retry; SIMULATED ONLY");
}
