/* SPDX-License-Identifier: MIT */
// The runner extracts the real service/host simulation fixture before main.
#include "resident-service-fixture.inc"
#include "../PioneerQwen35DataflowResidentOwnedRunner.h"
#include <vector>

struct ResidentHandle {
    ResidentRaw raw;ResidentScope scope;Host host;ResidentRun run;
    ResidentHandle():host(raw,scope),run(host) {}
};
struct RunnerController : ServiceController {
    std::vector<std::unique_ptr<ResidentHandle>> handles;
    std::vector<std::unique_ptr<DataflowProjectionAuthority>> candidates;
    std::vector<DataflowProjectionAuthority*> deletedCandidatePointers;
    bool failAllocation=false,denyCommit=false,corruptCommit=false,ownerRetained=false;
    unsigned deletedRuns=0,deletedCandidates=0,restoreChecks=0;
    bool restoreProof=false,mutateLive=false,reenter=false;unsigned reentryChecks=0;
    DataflowProjectionAuthority** liveSlot=nullptr;DataflowResidentGraphSession* recoverySession=nullptr;
    DataflowResidentOwnedRunner<ResidentHandle>* recoveryRunner=nullptr;
    RunnerController(DataflowServiceOwner& s,const DataflowStreamPlan& p):ServiceController{s,p} {}
    ResidentHandle* NewResidentRun() {
        handles.emplace_back(std::make_unique<ResidentHandle>());
        auto* handle=handles.back().get();raw=&handle->raw;run=&handle->run;pauses=0;return handle;
    }
    DataflowProjectionAuthority* NewResidentCandidate() {
        if (failAllocation) return nullptr;
        candidates.emplace_back(std::make_unique<DataflowProjectionAuthority>());return candidates.back().get();
    }
    void DeleteResidentRun(ResidentHandle*) { ++deletedRuns; }
    void DeleteResidentCandidate(DataflowProjectionAuthority* candidate) { assert(candidate);++deletedCandidates;deletedCandidatePointers.push_back(candidate); }
    bool ResidentCommitAllowed(const DataflowResidentGraphSession& session,const ResidentRun& current) {
        if (corruptCommit) assert(candidates.back()->InvalidateActivation(0));
        return !denyCommit && service.OwnsResidentRun(&session,current);
    }
    bool ResidentFleetRestored(const DataflowResidentGraphSession& session,const DataflowServiceOwner& current,
        const ResidentHandle& handle,DataflowProjectionAuthority* live,DataflowProjectionAuthority* candidate,
        DataflowProjectionAuthority* retired) {
        ++restoreChecks;
        assert(&current==&service && owner==&session && &handle.run==run && candidate && live);
        if (reenter) {
            assert(recoveryRunner && recoverySession && liveSlot);
            ++reentryChecks;
            assert(!recoveryRunner->ReleaseRetainedAfterRestoration(*recoverySession,service,*liveSlot,*this));
        }
        if (mutateLive && liveSlot) *liveSlot=(live==candidate?retired:candidate);
        return restoreProof;
    }
    void RetainResidentOwnedRun(const DataflowResidentGraphSession& session,ResidentHandle& handle,bool serviceRetained) {
        assert(serviceRetained && service.OwnsRetainedResidentRun(&session,handle.run));ownerRetained=true;
    }
};
int main() {
    (void)&q35_service_owner_fixture_main;(void)&q35_pci_fixture_main;(void)&q35_publisher_fixture_main;
    for (unsigned scenario=0;scenario<6;scenario++) {
        auto fixture=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
        assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
            fixture->rope,fixture->routes,99,DataflowGraphDecodeSteps,*plan));
        auto service=std::make_unique<DataflowServiceOwner>();auto serviceIO=std::make_unique<MockDataflowIO>();
        uint32_t code[96]{},dispatch[80]{};
        const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
        assert(service->Install(image,*serviceIO)==DataflowServiceComplete);
        auto initial=std::make_unique<DataflowProjectionAuthority>();
        assert(initial->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
        DataflowProjectionAuthority* live=initial.get();auto scratch=std::make_unique<DataflowProjectionProgram>();
        DataflowResidentGraphSession session;RunnerController io(*service,*plan);
        assert(session.Bind(*plan,fixture->routes,0,live,io));
        DataflowResidentOwnedRunner<ResidentHandle> runner;
        DataflowArgmaxResult output{};unsigned stage=0;
        if (!scenario) {
            assert(runner.Run(session,*service,live,1,42,true,3,*scratch,io,output,stage)==DataflowResidentSessionPreflight);
            assert(output.token==Vocabulary && session.NextTrigger()==1 && service->ResidentSessionIdle(&session));
            for (unsigned position=0;position<2;position++) {
                assert(runner.Run(session,*service,live,position,42+position,true,3,*scratch,io,output,stage)
                    ==DataflowResidentSessionComplete);
                assert(output.token==42 && !runner.Retained() && service->NextSequence()==position+1);
            }
            assert(io.deletedRuns==3 && io.deletedCandidates==3);
        } else {
            if (scenario==1) io.deliver=false;
            if (scenario==2) io.denyCommit=true;
            if (scenario==3) io.corruptCommit=true;
            if (scenario==4) io.failAllocation=true;
            if (scenario==5) io.safe=false;
            const auto result=runner.Run(session,*service,live,0,42,true,3,*scratch,io,output,stage);
            if (scenario==4 || scenario==5) {
                assert(result==(scenario==4?DataflowResidentSessionPreflight:DataflowResidentSessionInvalid)
                    && !runner.Retained() && service->ResidentSessionIdle(&session));
            } else {
                assert(result==DataflowResidentSessionRetained && runner.Retained() && io.ownerRetained
                    && service->Faulted() && output.token==Vocabulary && runner.RetainedHandle()
                    && runner.RetainedCandidate());
                if (scenario==2 || scenario==3) assert(runner.RetainedRetired()==initial.get());
                assert(runner.Run(session,*service,live,0,42,true,3,*scratch,io,output,stage)
                    ==DataflowResidentSessionInvalid);
                auto* originalLive=live;DataflowResidentGraphSession wrongSession;
                auto wrongService=std::make_unique<DataflowServiceOwner>();
                assert(!runner.ReleaseRetainedAfterRestoration(session,*wrongService,live,io)
                    && !io.restoreChecks && runner.Retained());
                DataflowProjectionAuthority* absent=nullptr;
                assert(!runner.ReleaseRetainedAfterRestoration(session,*service,absent,io)
                    && !io.restoreChecks && runner.Retained());
                assert(!runner.ReleaseRetainedAfterRestoration(wrongSession,*service,live,io)
                    && !io.restoreChecks && !io.deletedRuns && !io.deletedCandidates);
                assert(!runner.ReleaseRetainedAfterRestoration(session,*service,live,io)
                    && runner.Retained() && io.restoreChecks==1 && !io.deletedRuns && !io.deletedCandidates);
                io.restoreProof=true;io.mutateLive=true;io.liveSlot=&live;
                assert(!runner.ReleaseRetainedAfterRestoration(session,*service,live,io)
                    && runner.Retained() && !io.deletedRuns && !io.deletedCandidates && live!=originalLive);
                live=originalLive;io.mutateLive=false;io.reenter=true;
                io.recoverySession=&session;io.recoveryRunner=&runner;
                assert(runner.ReleaseRetainedAfterRestoration(session,*service,live,io)
                    && !runner.Retained() && !runner.RetainedHandle() && !runner.RetainedCandidate()
                    && session.Faulted() && !session.RetainedRun()
                    && service->ResidentRecoveredForRetirement(&session)
                    && service->Close(*serviceIO)==DataflowServiceInvalidState
                    && io.deletedRuns==1 && io.deletedCandidates==1 && io.reentryChecks==1
                    && io.deletedCandidatePointers.size()==1 && io.deletedCandidatePointers[0]!=live);
                assert(!runner.ReleaseRetainedAfterRestoration(session,*service,live,io));
            }
        }
    }
    puts("PASS resident owned runner requires restoration proof before retained-run retirement; SIMULATED ONLY");
}
