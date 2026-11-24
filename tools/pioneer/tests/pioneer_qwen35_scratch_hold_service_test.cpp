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
    bool holdAllowed=false,resumeAllowed=false,heldAllowed=true;
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

int main()
{
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    int scratch=0,other=0;
    for (unsigned scenario=0;scenario<5;scenario++) {
        auto io=std::make_unique<MockDataflowIO>();
        auto service=std::make_unique<DataflowServiceOwner>();
        ScratchHoldScope scope{&scratch,false,false};
        assert(service->Install(image,*io)==DataflowServiceComplete && service->Ready());
        assert(service->CloseForScratch(*io,&scratch,scope)==DataflowServiceInvalidState
            && service->Ready() && !io->roleStops && !io->resumeCount);
        scope.holdAllowed=true;
        if (scenario==1) io->failStopAudit=true;
        if (scenario==3) io->failRoleRestore=true;
        if (scenario==4) scope.heldAllowed=false;
        const auto held=service->CloseForScratch(*io,&scratch,scope);
        if (scenario==1) {
            assert(held==DataflowServiceStopAuditFailed && service->Faulted()
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
        if (scenario==4) {
            assert(held==DataflowServiceStopAuditFailed && service->Faulted()
                && io->roleRestores==1 && !io->resumeCount
                && service->Close(*io)==DataflowServiceInvalidState
                && service->CloseForScratch(*io,&scratch,scope)==DataflowServiceInvalidState);
            for (unsigned core=0;core<DataflowCoreCount;core++) assert(io->captured[core] && !io->released[core]);
            continue;
        }
        assert(held==DataflowServiceScratchHeld && service->ScratchHeld() && !service->Closed()
            && io->roleStops==1 && io->roleRestores==1 && !io->resumeCount);
        for (unsigned core=0;core<DataflowCoreCount;core++) assert(io->captured[core] && !io->released[core]);
        assert(service->Close(*io)==DataflowServiceInvalidState
            && service->FinishScratchClose(*io,&other,scope)==DataflowServiceInvalidState
            && service->FinishScratchClose(*io,&scratch,scope)==DataflowServiceInvalidState
            && !io->resumeCount);
        scope.resumeAllowed=true;io->failResume=scenario==2;
        const auto resumed=service->FinishScratchClose(*io,&scratch,scope);
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
    std::puts("PASS scratch-held close blocks resume until owner proof and retains uncertain failures; SIMULATED ONLY");
}
