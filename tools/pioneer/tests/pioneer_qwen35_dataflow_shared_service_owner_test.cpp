/* SPDX-License-Identifier: MIT */
// Exercises the actual service owner's shared-role lifecycle using fake IO.
// No role images, resets, PCI mappings or physical hardware are involved.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define main qwen35_single_service_test_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#include <memory>

int main()
{
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        assert(owner->Install(image,*io)==DataflowServiceComplete && owner->Ready());
        assert(io->roleStages==1 && io->roleArms==1 && io->roleStarts==DataflowCoreCount);
        io->failRoleRestore=true;
        assert(owner->Close(*io)==DataflowServiceRestoreFailed && owner->Faulted());
        assert(io->roleStops==1 && io->restoreCount==0 && io->resumeCount==0);
        io->failRoleRestore=false;
        assert(owner->Close(*io)==DataflowServiceClosed && owner->Closed());
        assert(io->roleStops==2 && io->roleRestores==2 && io->roleVerifies==1);
        assert(io->restoreCount==DataflowCoreCount && io->resumeCount==1);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        io->delayedRoleStartupCore=47; io->delayedRoleStartupReads=2;
        io->transientSnapshotCore=47; io->transientSnapshotFailures=2;
        assert(owner->Install(image,*io)==DataflowServiceComplete && owner->Ready());
        assert(io->roleStartupReads>=4 && !io->transientSnapshotFailures);
        assert(owner->Close(*io)==DataflowServiceClosed);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        io->staleStartupSequenceCore=47;
        assert(owner->Install(image,*io)==DataflowServiceReadyFailed && owner->Faulted());
        assert(owner->Diagnostic().stage==DataflowServiceDiagnosticWorkerProof
            && owner->Diagnostic().core==47 && !io->resumeCount);
        assert(owner->Close(*io)==DataflowServiceClosed);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        io->missingRoleStartupCore=47;
        assert(owner->Install(image,*io)==DataflowServiceReadyFailed && owner->Faulted());
        assert(owner->Diagnostic().stage==DataflowServiceDiagnosticWorkerProof
            && owner->Diagnostic().core==47 && !io->resumeCount);
        assert(owner->Close(*io)==DataflowServiceClosed);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        io->failRoleStage=true;
        assert(owner->Install(image,*io)==DataflowServiceSnapshotFailed && owner->Faulted());
        assert(io->roleStages==1 && !io->roleArms && !io->roleStarts);
        assert(owner->Close(*io)==DataflowServiceClosed && io->roleRestores==1);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        io->corruptRoleMailboxCore=47;
        assert(owner->Install(image,*io)==DataflowServiceStartupReadbackFailed && owner->Faulted());
        assert(!io->roleArms && !io->roleStarts);
        assert(owner->Close(*io)==DataflowServiceClosed && io->roleRestores==1);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        io->failRoleArm=true;
        assert(owner->Install(image,*io)==DataflowServiceStartFailed && owner->Faulted());
        assert(io->roleArms==1 && !io->roleStarts);
        assert(owner->Close(*io)==DataflowServiceClosed && io->roleRestores==1);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        io->corruptPriorResidentOnBoot=true;
        assert(owner->Install(image,*io)==DataflowServiceStartFailed && owner->Faulted());
        assert(io->roleArms==1 && !io->roleStarts);
        assert(owner->Close(*io)==DataflowServiceClosed && io->roleRestores==1);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        io->failStartCore=37;
        assert(owner->Install(image,*io)==DataflowServiceStartFailed && owner->Faulted());
        assert(io->roleArms==1 && io->roleStarts==37 && !io->resumeCount);
        assert(owner->Close(*io)==DataflowServiceClosed && io->roleRestores==1);
        assert(io->roleStarts==37 && io->resumeCount==1);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        assert(owner->Install(image,*io)==DataflowServiceComplete);
        io->failStopAudit=true;
        assert(owner->Close(*io)==DataflowServiceStopAuditFailed && owner->Faulted());
        assert(!io->roleRestores && !io->restoreCount && !io->resumeCount);
        io->failStopAudit=false;
        assert(owner->Close(*io)==DataflowServiceClosed && io->resumeCount==1);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        assert(owner->Install(image,*io)==DataflowServiceComplete);
        io->failRoleFinalize=true;
        assert(owner->Close(*io)==DataflowServiceRestoreFailed && owner->Faulted());
        assert(!io->resumeCount);
        io->failRoleFinalize=false;
        assert(owner->Close(*io)==DataflowServiceClosed && io->resumeCount==1);
    }
    {
        auto io=std::make_unique<MockDataflowIO>();
        auto owner=std::make_unique<DataflowServiceOwner>();
        assert(owner->Install(image,*io)==DataflowServiceComplete);
        io->failRoleVerify=true;
        assert(owner->Close(*io)==DataflowServiceRestoreFailed && owner->Faulted());
        assert(io->roleVerifies==1 && !io->resumeCount);
        io->failRoleVerify=false;
        assert(owner->Close(*io)==DataflowServiceClosed && io->roleVerifies==2);
        assert(io->resumeCount==1);
    }
    std::puts("PASS shared service stages before release and retains partial starts/restores; SIMULATED ONLY");
}
