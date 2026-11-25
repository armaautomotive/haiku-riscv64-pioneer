/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS 1
#define main q35_existing_service_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#include <memory>
struct StopIO : MockDataflowIO {
    DataflowServiceOwner* service=nullptr;
    bool sawClean=false;
    bool SharedStopAndHoldAll(unsigned& stage,unsigned& core) {
        sawClean=service && service->SharedSnapshotCleanStopPending();
        return MockDataflowIO::SharedStopAndHoldAll(stage,core);
    }
};
struct HostStub { bool Issued() const { return false; } bool IsRetained() const { return false; }
    bool IsAcknowledged() const { return false; } };
struct RunStub { bool attempted=false; HostStub host; };
struct CloseScope {
    const void* expected=nullptr; bool allowed=false; unsigned checks=0;
    bool ResidentCloseAllowed(const void* session,const DataflowServiceOwner& service,StopIO&) {
        ++checks; return allowed && session==expected && service.ResidentSessionIdle(session);
    }
};
int main() {
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    int session=0,other=0;
    for (unsigned mode=0;mode<4;mode++) {
        auto io=std::make_unique<StopIO>();auto service=std::make_unique<DataflowServiceOwner>();
        io->service=service.get();CloseScope scope{&session,false,0};
        assert(service->Install(image,*io)==DataflowServiceComplete && service->BindResidentSession(&session));
        assert(service->CloseResident(*io,&other,scope)==DataflowServiceInvalidState && scope.checks==0);
        assert(service->CloseResident(*io,&session,scope)==DataflowServiceInvalidState && scope.checks==1);
        assert(service->ResidentSessionIdle(&session) && !io->roleStops && !io->resumeCount);
        scope.allowed=true;
        RunStub run; assert(service->BeginResidentRun(&session,run,1));
        assert(service->CloseResident(*io,&session,scope)==DataflowServiceInvalidState && scope.checks==1);
        assert(service->CancelResidentRun(&session,run) && service->ResidentSessionIdle(&session));
        io->failStopAudit=mode==1;io->failRoleRestore=mode==2;io->failResume=mode==3;
        const auto result=service->CloseResident(*io,&session,scope);
        assert(io->sawClean);
        if (!mode) {
            assert(result==DataflowServiceClosed && service->Closed() && io->resumeCount==1);
            assert(service->Close(*io)==DataflowServiceClosed && io->resumeCount==1);
        }
        else {
            assert(service->Faulted() && !service->Closed());
            assert(result==(mode==1?DataflowServiceStopAuditFailed:mode==2?DataflowServiceRestoreFailed:DataflowServiceResumeFailed));
            const auto stops=io->roleStops,resumes=io->resumeCount;
            assert(service->Close(*io)==DataflowServiceInvalidState);
            assert(service->CloseResident(*io,&session,scope)==DataflowServiceInvalidState);
            assert(stops==io->roleStops && resumes==io->resumeCount);
        }
    }
    puts("PASS resident clean close retains uncertain stops; SIMULATED ONLY");
}
