/* SPDX-License-Identifier: MIT */
// Actual service lifecycle against fake device IO; no hardware access.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define main qwen35_existing_service_test_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#include <memory>

struct ScratchHoldScope {
    const void* expected=nullptr;
    bool holdAllowed=false,resumeAllowed=false,heldAllowed=true,heldRouteAllowed=true;
    unsigned heldRoutes=0,route=119;
    bool QualificationRestoreHeldRoute(const void* owner,const DataflowServiceOwner& service,MockDataflowIO&) {
        ++heldRoutes;
        if (owner!=expected || !heldRouteAllowed || !service.ScratchHeld()) return false;
        route=0;return true;
    }
    bool QualificationHoldAllowed(const void* owner,const DataflowServiceOwner& service) const {
        return owner==expected && holdAllowed && service.Ready();
    }
    bool QualificationResumeAllowed(const void* owner,const DataflowServiceOwner& service) const {
        return owner==expected && resumeAllowed && service.ScratchHeld();
    }
    bool QualificationHeldAllowed(const void* owner,const DataflowServiceOwner& service) const {
        return owner==expected && heldAllowed && service.ScratchHeld();
    }
};

struct SnapshotStopIO : MockDataflowIO {
    DataflowServiceOwner* owner=nullptr;
    bool observedCleanStop=false;
    bool observedStopDuringResume=false;
    bool SharedStopAndHoldAll(unsigned& stage,unsigned& core) {
        observedCleanStop=owner && owner->SharedSnapshotCleanStopPending();
        return MockDataflowIO::SharedStopAndHoldAll(stage,core);
    }
    bool ResumeRestoredFleet(unsigned& stage,unsigned& core,unsigned& detail) {
        observedStopDuringResume=owner && owner->SharedSnapshotCleanStopPending();
        return MockDataflowIO::ResumeRestoredFleet(stage,core,detail);
    }
};

int main()
{
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    int scratch=0,other=0;
    for (unsigned scenario=0;scenario<6;scenario++) {
        auto io=std::make_unique<SnapshotStopIO>();
        auto service=std::make_unique<DataflowServiceOwner>();
        io->owner=service.get();
        ScratchHoldScope scope{&scratch,false,false};
        assert(service->Install(image,*io)==DataflowServiceComplete && service->Ready());
        assert(service->CloseForScratch(*io,&scratch,scope)==DataflowServiceInvalidState
            && service->Ready() && !io->roleStops && !io->resumeCount);
        scope.holdAllowed=true;
        if (scenario==1) io->failStopAudit=true;
        if (scenario==3) io->failRoleRestore=true;
        if (scenario==4) scope.heldAllowed=false;
        if (scenario==5) scope.heldRouteAllowed=false;
        const auto held=service->CloseForScratch(*io,&scratch,scope);
        assert(io->observedCleanStop);
        if (scenario==1) {
            assert(held==DataflowServiceStopAuditFailed && service->Faulted()
                && service->SharedSnapshotCleanStopPending()
                && !io->resumeCount && service->Close(*io)==DataflowServiceInvalidState
                && service->CloseForScratch(*io,&scratch,scope)==DataflowServiceInvalidState);
            continue;
        }
        if (scenario==3) {
            assert(held==DataflowServiceRestoreFailed && service->Faulted()
                && io->roleRestores==1 && !io->resumeCount
                && service->Close(*io)==DataflowServiceInvalidState
                && service->CloseForScratch(*io,&scratch,scope)==DataflowServiceInvalidState
                && io->roleRestores==1);
            continue;
        }
        if (scenario==4 || scenario==5) {
            assert(held==DataflowServiceStopAuditFailed && service->Faulted()
                && io->roleRestores==1 && !io->resumeCount
                && service->Close(*io)==DataflowServiceInvalidState
                && service->CloseForScratch(*io,&scratch,scope)==DataflowServiceInvalidState);
            assert(scope.heldRoutes==1 && scope.route==(scenario==5?119U:0U));
            for (unsigned core=0;core<DataflowCoreCount;core++) assert(io->captured[core] && !io->released[core]);
            continue;
        }
        assert(scope.heldRoutes==1 && scope.route==0);
        assert(held==DataflowServiceScratchHeld && service->ScratchHeld() && !service->Closed()
            && !service->SharedSnapshotCleanStopPending()
            && io->roleStops==1 && io->roleRestores==1 && !io->resumeCount);
        for (unsigned core=0;core<DataflowCoreCount;core++) assert(io->captured[core] && !io->released[core]);
        assert(service->Close(*io)==DataflowServiceInvalidState
            && service->FinishScratchClose(*io,&other,scope)==DataflowServiceInvalidState
            && service->FinishScratchClose(*io,&scratch,scope)==DataflowServiceInvalidState
            && !io->resumeCount);
        scope.resumeAllowed=true;io->failResume=scenario==2;
        const auto resumed=service->FinishScratchClose(*io,&scratch,scope);
        assert(!io->observedStopDuringResume);
        if (scenario==2) {
            assert(resumed==DataflowServiceResumeFailed && service->Faulted() && !service->Closed()
                && io->resumeCount==1 && service->Close(*io)==DataflowServiceInvalidState
                && service->FinishScratchClose(*io,&scratch,scope)==DataflowServiceInvalidState
                && io->resumeCount==1);
            for (unsigned core=0;core<DataflowCoreCount;core++) assert(io->captured[core] && !io->released[core]);
        } else {
            assert(resumed==DataflowServiceClosed && service->Closed() && !service->ScratchHeld()
                && io->resumeCount==1);
            for (unsigned core=0;core<DataflowCoreCount;core++) assert(!io->captured[core] && io->released[core]);
        }
    }
    {
        auto io=std::make_unique<SnapshotStopIO>();
        auto service=std::make_unique<DataflowServiceOwner>();io->owner=service.get();
        assert(service->Install(image,*io)==DataflowServiceComplete && !service->SharedSnapshotCleanStopPending());
        io->failStopAudit=true;
        assert(service->Close(*io)==DataflowServiceStopAuditFailed && io->observedCleanStop
            && service->SharedSnapshotCleanStopPending());
        io->failStopAudit=false;io->observedCleanStop=false;
        assert(service->Close(*io)==DataflowServiceClosed && io->observedCleanStop
            && !service->SharedSnapshotCleanStopPending() && !io->observedStopDuringResume);
    }
    {
        auto io=std::make_unique<SnapshotStopIO>();
        auto service=std::make_unique<DataflowServiceOwner>();io->owner=service.get();
        io->failStartCore=37;
        assert(service->Install(image,*io)==DataflowServiceStartFailed && service->Faulted());
        assert(!service->SharedSnapshotCleanStopPending());
        assert(service->Close(*io)==DataflowServiceClosed && !io->observedCleanStop);
    }
    std::puts("PASS scratch-held close blocks resume until owner proof and retains uncertain failures; SIMULATED ONLY");
}
