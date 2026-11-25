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
    bool failAllocation=false,denyCommit=false,corruptCommit=false,ownerRetained=false;
    unsigned deletedRuns=0,deletedCandidates=0;
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
    void DeleteResidentCandidate(DataflowProjectionAuthority* candidate) { assert(candidate);++deletedCandidates; }
    bool ResidentCommitAllowed(const DataflowResidentGraphSession& session,const ResidentRun& current) {
        if (corruptCommit) assert(candidates.back()->InvalidateActivation(0));
        return !denyCommit && service.OwnsResidentRun(&session,current);
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
            }
        }
    }
    puts("PASS resident owned runner commits service sequence before exposing output; SIMULATED ONLY");
}
