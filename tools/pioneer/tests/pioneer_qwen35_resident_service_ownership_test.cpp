/* SPDX-License-Identifier: MIT */
// Actual service owner, session, model transaction and host PCI transaction;
// role/device behavior and the fleet audit are simulated.
#define main q35_service_owner_fixture_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#include "resident-host-fixture.inc"
#include "../PioneerQwen35DataflowResidentGraphSession.h"

using ResidentRun=DataflowResidentSessionRun<Host>;
struct ServiceController {
    DataflowServiceOwner& service;
    const DataflowStreamPlan& plan;
    const DataflowResidentGraphSession* owner=nullptr;
    ResidentRun* run=nullptr;ResidentRaw* raw=nullptr;
    bool retain=false,deliver=true,safe=true;unsigned pauses=0;
    bool ResidentBindAllowed(const DataflowResidentGraphSession&,const DataflowStreamPlan& requested,unsigned slot) {
        return &requested==&plan && !slot && service.Ready();
    }
    bool ReserveResidentSession(const DataflowResidentGraphSession& session,const DataflowStreamPlan& requested,unsigned slot) {
        if (!ResidentBindAllowed(session,requested,slot) || !service.BindResidentSession(&session)) return false;
        owner=&session;return true;
    }
    bool ResidentSessionSafe(const DataflowResidentGraphSession& session,const DataflowStreamPlan& requested,unsigned slot) {
        return safe && owner==&session && &requested==&plan && !slot && run && service.OwnsResidentRun(&session,*run);
    }
    void RetainResidentSession(const DataflowResidentGraphSession& session,const void* failed) {
        assert(owner==&session && failed==run && service.RetainResidentRun(&session,*run));retain=true;
    }
    void Pause() { if (deliver && ++pauses==1) PutCandidate(*raw,run->host.Request()); }
};
int main() {
    (void)&q35_service_owner_fixture_main;(void)&q35_pci_fixture_main;(void)&q35_publisher_fixture_main;
    auto fixture=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
        fixture->rope,fixture->routes,99,DataflowGraphDecodeSteps,*plan));
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    for (unsigned scenario=0;scenario<5;scenario++) {
        auto service=std::make_unique<DataflowServiceOwner>();auto serviceIO=std::make_unique<MockDataflowIO>();
        assert(service->Install(image,*serviceIO)==DataflowServiceComplete && service->NextSequence()==0);
        auto liveOwner=std::make_unique<DataflowProjectionAuthority>();
        assert(liveOwner->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
        DataflowProjectionAuthority* live=liveOwner.get();auto scratch=std::make_unique<DataflowProjectionProgram>();
        DataflowResidentGraphSession session;ServiceController controller{*service,*plan};
        assert(session.Bind(*plan,fixture->routes,0,live,controller));
        assert(!service->Ready() && service->ResidentSessionIdle(&session)
            && service->Close(*serviceIO)==DataflowServiceInvalidState);
        const unsigned iterations=scenario?1:3;
        for (unsigned step=0;step<iterations;step++) {
            auto raw=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*raw,scope);
            ResidentRun run(host);auto candidate=std::make_unique<DataflowProjectionAuthority>();
            controller.raw=raw.get();controller.run=&run;controller.pauses=0;
            assert(!service->BeginResidentRun(&session,run,session.NextTrigger()+1));
            assert(service->BeginResidentRun(&session,run,session.NextTrigger()));
            assert(service->GraphRunning() && !service->CommitResidentRun(&session,run)
                && service->Close(*serviceIO)==DataflowServiceInvalidState);
            if (scenario==1) raw->drop=1;
            if (scenario==2) scope.audit=false;
            DataflowArgmaxResult output{};DataflowProjectionAuthority* retired=nullptr;unsigned stage=0;
            const bool preflight=!scenario && !step;
            const unsigned position=preflight?1:step-unsigned(!scenario);
            const auto result=session.Run(run,*candidate,position,42+position,!preflight,3,*scratch,
                controller,output,retired,stage);
            if (preflight) {
                assert(result==DataflowResidentSessionPreflight && !host.Issued()
                    && service->CancelResidentRun(&session,run) && service->ResidentSessionIdle(&session)
                    && session.NextTrigger()==1 && !service->NextSequence());
            } else if (!scenario) {
                assert(result==DataflowResidentSessionComplete && output.token==42 && retired==liveOwner.get());
                assert(!service->CancelResidentRun(&controller,run));
                assert(service->CommitResidentRun(&session,run) && service->ResidentSessionIdle(&session)
                    && service->NextSequence()==step && session.NextTrigger()==step+1);
                liveOwner.swap(candidate);
            } else if (scenario>=3) {
                assert(result==DataflowResidentSessionComplete && host.IsAcknowledged()
                    && retired==liveOwner.get() && live==candidate.get());
                if (scenario==3) {
                    assert(candidate->ActivationReady(0) && candidate->InvalidateActivation(0));
                    assert(!service->CommitResidentRun(&session,run));
                } else assert(!service->CancelResidentRun(&session,run));
                assert(service->OwnsRetainedResidentRun(&session,run) && service->Faulted()
                    && !service->CommitResidentRun(&session,run));
            } else {
                assert(result==DataflowResidentSessionRetained && controller.retain
                    && service->OwnsRetainedResidentRun(&session,run) && service->Faulted()
                    && !service->CancelResidentRun(&session,run) && !service->CommitResidentRun(&session,run)
                    && !retired && output.token==Vocabulary && live==liveOwner.get());
            }
        }
    }
    puts("PASS resident service reservation, sequence adoption, preflight retry and fault retention; SIMULATED ONLY");
}
