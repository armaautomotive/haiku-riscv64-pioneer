/* SPDX-License-Identifier: MIT */
// Actual resident session, metadata transaction and host PCI protocol over a
// simulated panel and fleet. The policy below grants simulation only.
#include "resident-host-fixture.inc"
#include "../PioneerQwen35DataflowResidentGraphSession.h"

struct ResidentController {
    const DataflowResidentGraphSession* owner=nullptr;
    const DataflowStreamPlan* sealed=nullptr;
    ResidentRaw* raw=nullptr;Host* host=nullptr;
    bool safe=true,retain=false,deliver=true,corrupt=false,loseAfterPause=false;
    unsigned pauses=0,safetyChecks=0,denyOn=UINT32_MAX;
    bool ResidentBindAllowed(const DataflowResidentGraphSession&,const DataflowStreamPlan& plan,unsigned slot) {
        return safe && &plan==sealed && slot==0;
    }
    bool ReserveResidentSession(const DataflowResidentGraphSession& session,const DataflowStreamPlan& plan,unsigned slot) {
        if (owner || !ResidentBindAllowed(session,plan,slot)) return false;
        owner=&session;return true;
    }
    bool ResidentSessionSafe(const DataflowResidentGraphSession& session,const DataflowStreamPlan& plan,unsigned slot) {
        return safe && ++safetyChecks!=denyOn && owner==&session && &plan==sealed && slot==0;
    }
    void RetainResidentSession(const DataflowResidentGraphSession& session,const void* run) {
        assert(owner==&session && run);retain=true;
    }
    void Pause() {
        ++pauses;
        if (deliver && pauses==1) {
            PutCandidate(*raw,host->Request());
            if (corrupt) raw->mailbox[0].reserved[5]^=1;
        }
        if (loseAfterPause) safe=false;
    }
};
int main() {
    (void)&q35_pci_fixture_main;(void)&q35_publisher_fixture_main;
    for (unsigned scenario=0;scenario<7;scenario++) {
        auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
        assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,
            99,DataflowGraphDecodeSteps,*plan));
        auto liveOwner=std::make_unique<DataflowProjectionAuthority>();
        assert(liveOwner->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
        DataflowProjectionAuthority* live=liveOwner.get();
        auto scratch=std::make_unique<DataflowProjectionProgram>();
        DataflowResidentGraphSession session;ResidentController controller;controller.sealed=plan.get();
        assert(session.Bind(*plan,f->routes,0,live,controller));
        if (!scenario) {
            // A valid command whose model position is not yet ready must not
            // consume the next worker trigger; the retry starts at one.
            auto rejectedCandidate=std::make_unique<DataflowProjectionAuthority>();
            auto rejectedRaw=std::make_unique<ResidentRaw>();ResidentScope rejectedScope;
            Host rejectedHost(*rejectedRaw,rejectedScope);DataflowResidentSessionRun<Host> rejected(rejectedHost);
            DataflowArgmaxResult rejectedOutput{};DataflowProjectionAuthority* rejectedRetired=nullptr;
            unsigned rejectedStage=0;
            assert(session.Run(rejected,*rejectedCandidate,1,42,true,3,*scratch,controller,
                rejectedOutput,rejectedRetired,rejectedStage)==DataflowResidentSessionPreflight);
            assert(!rejectedHost.Issued() && !session.Faulted() && !rejectedRetired);
        }
        const unsigned iterations=scenario?1:2;
        for (unsigned position=0;position<iterations;position++) {
            auto candidate=std::make_unique<DataflowProjectionAuthority>();
            auto raw=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*raw,scope);
            DataflowResidentSessionRun<Host> run(host);
            controller.raw=raw.get();controller.host=&host;controller.pauses=0;
            if (scenario==1) raw->drop=1;
            if (scenario==2) scope.audit=false;
            if (scenario==3) controller.deliver=false;
            if (scenario==4) controller.loseAfterPause=true;
            if (scenario==5) controller.corrupt=true;
            if (scenario==6) controller.denyOn=6; // after ACK, before authority promotion
            DataflowArgmaxResult output{};DataflowProjectionAuthority* retired=nullptr;unsigned stage=0;
            const bool decode=position || scenario;
            const auto result=session.Run(run,*candidate,position,42+position,decode,3,*scratch,
                controller,output,retired,stage);
            if (!scenario) {
                assert(result==DataflowResidentSessionComplete && retired==liveOwner.get()
                    && live==candidate.get() && output.token==(decode?42:Vocabulary));
                assert(!session.Faulted() && !controller.retain && host.IsAcknowledged());
                assert(host.Request().trigger==position+1);
                liveOwner.swap(candidate);
            } else {
                assert(result==DataflowResidentSessionRetained && session.Faulted()
                    && session.RetainedRun()==&run && controller.retain && !retired
                    && live==liveOwner.get() && output.token==Vocabulary);
                if (scenario==6) assert(host.IsAcknowledged());
                assert(session.Run(run,*candidate,position,42,decode,3,*scratch,
                    controller,output,retired,stage)==DataflowResidentSessionInvalid);
            }
        }
    }
    puts("PASS resident session, prompt/decode state promotion and one-shot fault retention; SIMULATED ONLY");
}
